#pragma once

// макросы
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=nullptr; } }
#endif
#ifndef SAFE_DELETE_MEMORY
#define SAFE_DELETE_MEMORY(p) { if (p) { free(p);   (p)=nullptr; } }
#endif
#ifndef SAFE_DELETE_OBJECT
#define SAFE_DELETE_OBJECT(p)   { if (p) { DeleteObject(p); (p)=nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=nullptr; } }
#endif
#ifndef SAFE_RELEASE_C
#define SAFE_RELEASE_C(p)      { if (p) { (p).Release(); } }
#endif
