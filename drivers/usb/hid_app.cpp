/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * USB HID keyboards and game controllers, ported from pico-speccy
 * (src/drivers/usbhost/hid_app.cpp): HID gamepads (generic SNES-style pads,
 * DualShock 4, DualSense, Logitech F710 in D mode and three clone pads by
 * VID:PID), the RIP report-descriptor parser fallback, robustness of the
 * report path (zero-length reports, short keyboard reports), release of the
 * keys of a keyboard that is unplugged, and the re-arm watchdog for HID
 * interfaces whose IN endpoint lost its pending transfer.
 * Not ported: the mouse (the BK has none here), consumer (media) keys, the OSD
 * "HID devices" page and the GET_REPORT stuck-key resync of the PIO-USB port.
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include "hid_rip.h"
#include "nespad.h"
#include "pico/time.h"
#include <string.h>

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

#define MAX_REPORT  4

// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
  uint16_t vid;
  uint16_t pid;
  bool     mounted;
  uint8_t  dev_addr;
  uint32_t report_total;   // reports received (re-arm watchdog liveness)
  uint8_t  pad;            // DPAD_* state of a game controller on this instance
}hid_info[CFG_TUH_HID];

void process_kbd_report(
  hid_keyboard_report_t const *report,
  hid_keyboard_report_t const *prev_report
);

static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

//--------------------------------------------------------------------+
// Game controllers -> nespad bits
//--------------------------------------------------------------------+

// Sony DualShock 4 (PS4 wired): VID=054C, PID=05C4 (v1) / 09CC (v2).
// 507-byte report descriptor with vendor-page reports overflows TinyUSB's
// parser, so these bypass the report info table and the 64-byte input report
// (id=0x01) is decoded by hand.
static inline bool is_dualshock4(uint16_t vid, uint16_t pid)
{
  return vid == 0x054C && (pid == 0x05C4 || pid == 0x09CC);
}

// Sony DualSense (PS5): VID=054C, PID=0CE6. Same descriptor-overflow problem as
// DS4; layout differs (L2/R2 analog at bytes 4-5, face/dpad at byte 7).
static inline bool is_dualsense(uint16_t vid, uint16_t pid)
{
  return vid == 0x054C && pid == 0x0CE6;
}

// "FEED:2320" clone gamepad / arcade stick: composite report id=0x07.
static inline bool is_gp_feed_2320(uint16_t vid, uint16_t pid)
{
  return vid == 0xFEED && pid == 0x2320;
}

// Generic Bluetooth gamepad clone (2563:0575, 8BitDo / cheap clones): simple
// report without id.
static inline bool is_gp_2563_0575(uint16_t vid, uint16_t pid)
{
  return vid == 0x2563 && pid == 0x0575;
}

// Logitech F710 / Cordless RumblePad 2 in DirectInput mode (switch "D"):
// 046D:C219, report id 0x01. In XInput mode (switch "X") it is 046D:C21F and
// xinput_host handles it.
static inline bool is_logitech_f710_dinput(uint16_t vid, uint16_t pid)
{
  return vid == 0x046D && pid == 0xC219;
}

// "Game Stick Lite" 2.4 GHz gamepad, DragonRise-based clone: 0810:0001. It
// announces boot keyboard + mouse interfaces, both carrying the same 8-byte
// joystick report.
static inline bool is_gp_0810_0001(uint16_t vid, uint16_t pid)
{
  return vid == 0x0810 && pid == 0x0001;
}

// One controller's state: directions, the main fire button (DPAD_A: the
// bottom/south face button, as the XInput mapping has it), the second one
// (DPAD_B), SELECT and START.
static void pad_set(uint8_t instance, bool up, bool down, bool left, bool right,
                    bool fire, bool alt, bool select, bool start)
{
  if (instance >= CFG_TUH_HID) return;
  uint8_t s = 0;
  if (up)     s |= DPAD_UP;
  if (down)   s |= DPAD_DOWN;
  if (left)   s |= DPAD_LEFT;
  if (right)  s |= DPAD_RIGHT;
  if (fire)   s |= DPAD_A;
  if (alt)    s |= DPAD_B;
  if (select) s |= DPAD_SELECT;
  if (start)  s |= DPAD_START;
  hid_info[instance].pad = s;
}

// xinput (joy_hid.cpp) and HID controllers together make the USB joystick
extern "C" uint8_t xinput_pad_state;

static void usbpad_update(void)
{
  uint8_t s = xinput_pad_state;
  for (uint8_t i = 0; i < CFG_TUH_HID; i++)
    if (hid_info[i].mounted) s |= hid_info[i].pad;
  usbpad_state = s;
}

extern "C" void usbpad_xinput_changed(void)
{
  usbpad_update();
}

// D-pad hat: 0=N, 1=NE ... 7=NW, 8+ = neutral
static void hat_dirs(uint8_t hat, bool* up, bool* down, bool* left, bool* right)
{
  *up    = hat == 7 || hat == 0 || hat == 1;
  *right = hat == 1 || hat == 2 || hat == 3;
  *down  = hat == 3 || hat == 4 || hat == 5;
  *left  = hat == 5 || hat == 6 || hat == 7;
}

// 8-bit analog stick, 0x80 = centre, ~50% deflection
static inline bool stick_lo(uint8_t v) { return v < 0x40; }
static inline bool stick_hi(uint8_t v) { return v > 0xC0; }

// Cheap SNES-style USB pads (e.g. 081F:E401) and other HID joysticks/gamepads:
//   [0] X axis : 0x00=Left, 0x7F=center, 0xFF=Right
//   [1] Y axis : 0x00=Up,   0x7F=center, 0xFF=Down
//   [5] high nibble = face buttons : 0x10=X, 0x20=A, 0x40=B, 0x80=Y
//   [6] shoulders/system           : 0x01=L, 0x02=R, 0x10=Select, 0x20=Start
static void process_hid_gamepad(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  pad_set(instance,
          stick_lo(report[1]), stick_hi(report[1]), stick_lo(report[0]), stick_hi(report[0]),
          (report[5] & 0x40) != 0,   // B (bottom) = fire
          (report[5] & 0x20) != 0,   // A
          (report[6] & 0x10) != 0,
          (report[6] & 0x20) != 0);
}

// DualShock 4, after the report id 0x01:
//   [0..3] LX, LY, RX, RY (0x80 = center)
//   [4]    low nibble = D-pad hat; high nibble = Square 0x10, Cross 0x20,
//          Circle 0x40, Triangle 0x80
//   [5]    L1 0x01, R1 0x02, L2 0x04, R2 0x08, Share 0x10, Options 0x20
static void process_ds4_gamepad(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  bool u, d, l, r;
  hat_dirs(report[4] & 0x0F, &u, &d, &l, &r);
  pad_set(instance,
          u || stick_lo(report[1]), d || stick_hi(report[1]),
          l || stick_lo(report[0]), r || stick_hi(report[0]),
          (report[4] & 0x20) != 0,   // Cross
          (report[4] & 0x40) != 0,   // Circle
          (report[5] & 0x10) != 0,   // Share
          (report[5] & 0x20) != 0);  // Options
}

// DualSense, after the report id 0x01:
//   [0..3] LX, LY, RX, RY (0x80 center), [4]/[5] L2/R2 analog, [6] counter
//   [7]    low nibble = D-pad hat; high nibble = Square 0x10, Cross 0x20,
//          Circle 0x40, Triangle 0x80
//   [8]    L1 0x01, R1 0x02, L2 0x04, R2 0x08, Create 0x10, Options 0x20
static void process_ds5_gamepad(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 9) return;
  bool u, d, l, r;
  hat_dirs(report[7] & 0x0F, &u, &d, &l, &r);
  pad_set(instance,
          u || stick_lo(report[1]), d || stick_hi(report[1]),
          l || stick_lo(report[0]), r || stick_hi(report[0]),
          (report[7] & 0x20) != 0,   // Cross
          (report[7] & 0x40) != 0,   // Circle
          (report[8] & 0x10) != 0,   // Create
          (report[8] & 0x20) != 0);  // Options
}

// Logitech F710 in D mode, after the report id 0x01:
//   [0..3] LX, LY, RX, RY (0x80 center)
//   [4]    low nibble = hat; high nibble = X 0x10, A 0x20, B 0x40, Y 0x80
//   [5]    LB 0x01, RB 0x02, LT 0x04, RT 0x08, BACK 0x10, START 0x20
static void process_f710_dinput(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  bool u, d, l, r;
  hat_dirs(report[4] & 0x0F, &u, &d, &l, &r);
  pad_set(instance,
          u || stick_lo(report[1]), d || stick_hi(report[1]),
          l || stick_lo(report[0]), r || stick_hi(report[0]),
          (report[4] & 0x20) != 0,   // A
          (report[4] & 0x40) != 0,   // B
          (report[5] & 0x10) != 0,   // BACK
          (report[5] & 0x20) != 0);  // START
}

// 2563:0575, simple report (no id):
//   [0] Y 0x01, B 0x02, A 0x04, X 0x08, L 0x10, R 0x20
//   [1] SELECT 0x01, START 0x02
//   [2] low nibble = hat; [3..6] LX, LY, RX, RY (0x80 center)
static void process_gp_2563_0575(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  bool u, d, l, r;
  hat_dirs(report[2] & 0x0F, &u, &d, &l, &r);
  pad_set(instance,
          u || stick_lo(report[4]), d || stick_hi(report[4]),
          l || stick_lo(report[3]), r || stick_hi(report[3]),
          (report[0] & 0x04) != 0,   // A
          (report[0] & 0x02) != 0,   // B
          (report[1] & 0x01) != 0,
          (report[1] & 0x02) != 0);
}

// FEED:2320, after the report id 0x07:
//   [0..3] LX, LY, RX, RY (0x80 center); [4] low nibble = hat
//   [5] A 0x01, B 0x02, Y 0x04, X 0x08, L 0x10, R 0x20
//   [6] SELECT 0x04, START 0x08
static void process_gp_feed_2320(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  bool u, d, l, r;
  hat_dirs(report[4] & 0x0F, &u, &d, &l, &r);
  pad_set(instance,
          u || stick_lo(report[1]), d || stick_hi(report[1]),
          l || stick_lo(report[0]), r || stick_hi(report[0]),
          (report[5] & 0x01) != 0,   // A
          (report[5] & 0x02) != 0,   // B
          (report[6] & 0x04) != 0,
          (report[6] & 0x08) != 0);
}

// 0810:0001, after the report id (0x01/0x02), 7 bytes:
//   [2] LX / crosspad X, [3] LY / crosspad Y (0x80 center)
//   [4] high nibble: Y 0x10, B 0x20, A 0x40, X 0x80
//   [5] LB 0x01, RB 0x02, LT 0x04, RT 0x08, Select 0x10, Start 0x20
static void process_gp_0810_0001(uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (len < 7) return;
  pad_set(instance,
          stick_lo(report[3]), stick_hi(report[3]), stick_lo(report[2]), stick_hi(report[2]),
          (report[4] & 0x20) != 0,   // B
          (report[4] & 0x40) != 0,   // A
          (report[5] & 0x10) != 0,
          (report[5] & 0x20) != 0);
}

//--------------------------------------------------------------------+
// Keyboard state
//--------------------------------------------------------------------+

static hid_keyboard_report_t prev_report = { 0 , 0 , {0}};
// the interface the keyboard state came from (released when it goes away)
static uint8_t kbd_dev_addr = 0;
static uint8_t kbd_instance = 0xFF;

static void kbd_report_apply(uint8_t dev_addr, uint8_t instance, hid_keyboard_report_t const* rpt)
{
  kbd_dev_addr = dev_addr;
  kbd_instance = instance;
  process_kbd_report(rpt, &prev_report);
  prev_report = *rpt;
}

// A keyboard that goes away with a key held would leave that key down for the
// emulated machine for ever.
static void kbd_detach(uint8_t dev_addr, uint8_t instance)
{
  if (dev_addr != kbd_dev_addr || instance != kbd_instance) return;
  kbd_instance = 0xFF;
  static const hid_keyboard_report_t released = { 0, 0, { 0 } };
  if (memcmp(&prev_report, &released, sizeof released) != 0) {
    process_kbd_report(&released, &prev_report);
    prev_report = released;
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  if (instance >= CFG_TUH_HID) return;

  // Interface protocol (hid_interface_protocol_enum_t)
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  uint16_t vid = 0, pid = 0;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  hid_info[instance].vid = vid;
  hid_info[instance].pid = pid;
  hid_info[instance].dev_addr = dev_addr;
  hid_info[instance].report_total = 0;
  hid_info[instance].pad = 0;
  hid_info[instance].report_count = 0;
  hid_info[instance].mounted = true;

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);

    // Fallback: some pads (long composite descriptors) trip the stock tinyusb
    // parser and return 0 reports. Re-parse with the RIP parser ported from
    // fruit-bat/pico-hid-host (via frank-nes and pico-speccy).
    if (hid_info[instance].report_count == 0 && desc_report && desc_len) {
      tuh_hid_report_info_plus_t rip_info[MAX_REPORT];
      uint8_t n = tuh_hid_parse_report_descriptor_plus(rip_info, MAX_REPORT, desc_report, desc_len);
      if (n > 0 && n <= MAX_REPORT) {
        for (uint8_t i = 0; i < n; i++) {
          hid_info[instance].report_info[i].report_id  = rip_info[i].report_id;
          hid_info[instance].report_info[i].usage      = (uint8_t)rip_info[i].usage;
          hid_info[instance].report_info[i].usage_page = rip_info[i].usage_page;
        }
        hid_info[instance].report_count = n;
      }
    }
  }

  // 0810:0001 announces boot kbd/mouse protocol but is a gamepad dongle.
  // TinyUSB sends SET_PROTOCOL(BOOT), which makes the dongle go silent:
  // switch it to the report protocol so it starts sending HID reports.
  if (is_gp_0810_0001(vid, pid) && itf_protocol != HID_ITF_PROTOCOL_NONE) {
    tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available;
  // a refused request is retried by hid_app_rearm_tick()
  tuh_hid_receive_report(dev_addr, instance);
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  if (instance < CFG_TUH_HID) {
    hid_info[instance].mounted = false;
    hid_info[instance].pad = 0;     // no direction or button stays held
    usbpad_update();
  }
  kbd_detach(dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if (instance >= CFG_TUH_HID) return;
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  // A zero-length interrupt-IN report is legal idle traffic for some devices (a
  // Dell wireless receiver sends ZLPs on its third HID interface instead of
  // NAKing); every dispatch below reads report[0].
  if (report == NULL || len == 0) {
    tuh_hid_receive_report(dev_addr, instance);
    return;
  }
  hid_info[instance].report_total++;

  if (is_gp_0810_0001(hid_info[instance].vid, hid_info[instance].pid)) {
    // registers with boot kbd/mouse protocol but sends joystick reports
    if (len >= 8 && (report[0] == 0x01 || report[0] == 0x02)) {
      process_gp_0810_0001(instance, report + 1, len - 1);
      usbpad_update();
    }
    tuh_hid_receive_report(dev_addr, instance);
    return;
  }

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
    {
      // Zero-padded copy rather than a cast: a report shorter than the boot
      // layout would otherwise be read past its end.
      hid_keyboard_report_t kb = { 0, 0, { 0 } };
      memcpy(&kb, report, len < sizeof(kb) ? len : sizeof(kb));
      kbd_report_apply(dev_addr, instance, &kb);
      break;
    }

    case HID_ITF_PROTOCOL_MOUSE:
      break; // no mouse on the BK

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
      usbpad_update();
      break;
  }

  // continue to request to receive report
  tuh_hid_receive_report(dev_addr, instance);
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint16_t const vid = hid_info[instance].vid;
  uint16_t const pid = hid_info[instance].pid;

  if (is_dualshock4(vid, pid) && len >= 10 && report[0] == 0x01) {
    process_ds4_gamepad(instance, report + 1, len - 1);
    return;
  }
  if (is_dualsense(vid, pid) && len >= 11 && report[0] == 0x01) {
    process_ds5_gamepad(instance, report + 1, len - 1);
    return;
  }
  if (is_logitech_f710_dinput(vid, pid) && len >= 8 && report[0] == 0x01) {
    process_f710_dinput(instance, report + 1, len - 1);
    return;
  }
  if (is_gp_2563_0575(vid, pid) && len >= 7) {
    process_gp_2563_0575(instance, report, len);
    return;
  }
  if (is_gp_feed_2320(vid, pid) && len >= 8 && report[0] == 0x07) {
    process_gp_feed_2320(instance, report + 1, len - 1);
    return;
  }

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info) return;

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h.
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
      {
        // Assume keyboard follow boot report layout. Zero-padded copy: the
        // report pointer was advanced past the id byte, so a short one would be
        // read past its end and its tail decoded as keycodes.
        hid_keyboard_report_t kb = { 0, 0, { 0 } };
        memcpy(&kb, report, len < sizeof(kb) ? len : sizeof(kb));
        kbd_report_apply(dev_addr, instance, &kb);
        break;
      }

      case HID_USAGE_DESKTOP_GAMEPAD:
      case HID_USAGE_DESKTOP_JOYSTICK:
        process_hid_gamepad(instance, report, len);
        break;

      default: break;
    }
  }
}

//--------------------------------------------------------------------+
// Re-arm watchdog
//--------------------------------------------------------------------+

// TinyUSB TU_ASSERT failures (tusb_config.h CFG_TUSB_DEBUG_BREAKPOINT): a
// counting no-op instead of a bkpt that freezes a session with a debug probe
// attached. An event queue overflow is one of them, and the only way an
// endpoint gets wedged.
volatile uint32_t g_tusb_assert_count = 0;
extern "C" void bk_tusb_assert_hook(void) { g_tusb_assert_count++; }

// The ONLY re-arm points are tuh_hid_mount_cb() and the tail of
// tuh_hid_report_received_cb(), so one failed tuh_hid_receive_report() leaves
// the interface permanently unarmed and the device silently dead until it is
// re-plugged. tuh_hid_receive_ready() is false while a transfer is pending, so
// in normal operation this does nothing. Called from keyboard_tick() (main
// loop context, never from a tuh callback); runs every 50 ms.
extern "C" void hid_app_rearm_tick(void)
{
  static uint32_t last_ms = 0;
  static uint32_t last_assert_seen = 0;
  static uint32_t last_reports[CFG_TUH_HID] = { 0 };
  static uint32_t stale_since_ms[CFG_TUH_HID] = { 0 };

  const uint32_t now = (uint32_t)(time_us_64() / 1000u);
  if ((uint32_t)(now - last_ms) < 50) return;
  last_ms = now;

  for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
    if (!hid_info[i].mounted) { stale_since_ms[i] = now; continue; }
    const uint8_t daddr = hid_info[i].dev_addr;

    // how long this interface has produced nothing
    if (hid_info[i].report_total != last_reports[i]) {
      last_reports[i] = hid_info[i].report_total;
      stale_since_ms[i] = now;
    }

    if (tuh_hid_receive_ready(daddr, i)) {
      // No transfer pending: the interface really is unarmed.
      tuh_hid_receive_report(daddr, i);
      continue;
    }

    // A transfer in flight is the normal state, also for a while after the HCD
    // finished (the completion event waits in the queue for tuh_task()). The
    // one thing that wedges an endpoint is a DROPPED event, and events are only
    // dropped by queue_event()'s TU_ASSERT: require the assert counter to have
    // moved AND the interface to have been silent for over a second.
    if (g_tusb_assert_count == last_assert_seen ||
        (uint32_t)(now - stale_since_ms[i]) <= 1000) continue;

    last_assert_seen = g_tusb_assert_count;
    tuh_hid_receive_abort(daddr, i);
    if (tuh_hid_receive_report(daddr, i)) stale_since_ms[i] = now;
  }
}
