#include "HTTPd.h"

#include "TCPSocket.h"
#include <osapi.h>
#include <mem.h>
#include <spi_flash.h>
#include <user_interface.h>
#include "str.h"
#include "UTF8_KOI8.h"


#define DEBUG(...)	os_printf(__VA_ARGS__)


#define FLASH_HTTP_DATA	0x70000
#define FLASH_HTTP_SIZE	(0x7C000-0x70000)
#define PUT_BUFFER_SIZE	1024


#define METHOD_PUT		'p'
#define METHOD_GET		'g'


HTTPd::HTTPd(int port)
    : TCPSocket(port)
{
    handler=0;
    put.path=0;
    put.buffer=0;
    get.path=0;
    get.buf2free=0;
}


HTTPd::~HTTPd()
{
    if (put.path) os_strdel(put.path);
    if (put.buffer) delete[] put.buffer;
    if (get.path) os_strdel(get.path);
    if (get.buf2free) os_free(get.buf2free);
}


void HTTPd::setHandler(HTTPHandler *h)
{
    handler=h;
}


HTTPd::HTTPd(TCPSocket *parent, struct espconn *c)
    : TCPSocket(parent, c)
{
    buflen=0;
    http_method=0;
    put.path=0;
    handler=((HTTPd*)parent)->handler;
    put.buffer=0;
    get.path=0;
    get.buf2free=0;
}


TCPSocket* HTTPd::clientConnected(struct espconn *c)
{
    DEBUG("HTTPd: new connection\n");
    return new HTTPd(this, c);
}


void HTTPd::dataReceived(const uint8_t *data, int len)
{
    DEBUG("HTTPd: data received size=%d\n", len);
again:
    if (len==0) return;
    
    if (! http_method)
    {
	// ������������ ��� ����� � ����� �� �� ������. ������ ��� ����� ���������
	while (len)
	{
    	    char cc=(char)(*data++); len--;
	    
    	    if (cc=='\r')
    	    {
        	// ����������
    	    } else
    	    if (cc=='\n')
    	    {
    		// ����� ������
        	buf[buflen]=0;
        	
        	// �������� ������ � ������ ��������
        	char buf_l[buflen+1];
        	for (int i=0; i<buflen; i++)
        	    buf_l[i]=to_lower(buf[i]);
        	buf_l[buflen]=0;
		
		// ���������� ����� ������
        	buflen=0;
		
        	DEBUG("HTTPd: '%s'\n", buf);
        	
        	// ������ ��� ���������
        	if (! os_strncmp(buf_l, "put /", 5))
        	{
        	    // �������
        	    char *path=buf+4;
        	    URL_UTF8_To_KOI8 (path, path);
        	    put.path=os_strdup(path);
        	    put.size=0;
        	} else
        	if (! os_strncmp(buf_l, "get /", 5))
        	{
        	    // ����������
        	    char *path=buf+4;
                URL_UTF8_To_KOI8 (path, path);
        	    get.path=os_strdup(path);
        	} else
        	if (! os_strncmp(buf_l, "content-length: ", 16))
        	{
        	    // ������ ��� �������
        	    put.contentLength=parse_int(buf+16);
        	} else
        	if (! buf[0])
        	{
        	    // ����� ����������
        	    if (get.path)
        	    {
        		// ��� GET
        		DEBUG("HTTPd: GET method\n");
        		http_method=METHOD_GET;
        		
        		// ���� � �����������
        		if (handler)
        		{
        		    const char *headers=handler->webGet(get.path, &get.addr, &get.size);
        		    if (headers)
        		    {
        			// ���� ����� � ������ - ���� ����� ��� �������
        			if ( (get.addr) && (!(get.addr & 0x80000000)) ) get.buf2free=(void*)get.addr;
        			
        			// ����� ���������� ���������
        			send((const uint8_t*)headers, os_strlen(headers));
        			delete[] headers;
        			return;
        		    }
        		}
        		
    			// ���� � �������� �������
    			if (! httpFS(get.path))
    			{
    			    // �� ������
                            os_strcpy(buf, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found");
                            send((const uint8_t*)buf, os_strlen(buf));
                            close();
    			}
    			return;
        	    } else
        	    if ( (put.path) && (put.contentLength>0) )
        	    {
        		// ��� PUT
        		DEBUG("HTTPd: PUT request for '%s' size=%d\n", put.path, put.contentLength);
        		http_method=METHOD_PUT;
        		
        		if (! os_strcmp(put.path, "/httpfs.bin"))
        		{
        		    // ���������� ������
        		    put.addr=FLASH_HTTP_DATA | 0x80000000;
        		} else
        		if (handler)
        		{
        		    // ����������
        		    put.addr=handler->webPutStart(put.path, put.contentLength);
        		    if (put.addr==0)
        		    {
        			// ������
        			static const char *str="HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found";
        			send((const uint8_t*)str, os_strlen(str));
        			close();
        			return;
        		    }
        		} else
        		{
        		    // ��� �����������
        		    static const char *str="HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found";
        		    send((const uint8_t*)str, os_strlen(str));
        		    close();
        		    return;
        		}
        		put.size=put.contentLength;
        		put.buffer=new uint8_t[PUT_BUFFER_SIZE];
        		put.bufferPos=0;
        		goto again;
        	    } else
        	    {
        		// ������
        		static const char *str="HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
        		send((const uint8_t*)str, os_strlen(str));
        		close();
        		return;
        	    }
        	}
    	    } else
    	    {
        	// ������ ������
        	if (buflen < (int)(sizeof(buf)-1)) buf[buflen++]=cc;
    	    }
	}
    } else
    if (http_method==METHOD_PUT)
    {
	// ������������ ������ PUT
	if (len > put.size) len=put.size;
	
	handler->webPutData(put.path, data, len);
	
	if (put.addr & 0x80000000)
	{
	    // ������ �� ����
	    while (len > 0)
	    {
    		// �������� � �����
    		int l=len;
    		if (l > PUT_BUFFER_SIZE-put.bufferPos) l=PUT_BUFFER_SIZE-put.bufferPos;
    		os_memcpy(put.buffer+put.bufferPos, data, l);
    		put.bufferPos+=l;
    		data+=l;
    		len-=l;
    		put.size-=l;
    		
    		if ( (put.bufferPos >= PUT_BUFFER_SIZE) || (put.size <= 0) )
    		{
    		    // ����� ������ ��� ������ PUT - ���� ��������
    		    
        	    // ��������� 0xff ��������� ����� ������
        	    if (put.bufferPos < PUT_BUFFER_SIZE)
            		os_memset(put.buffer+put.bufferPos, 0xff, PUT_BUFFER_SIZE-put.bufferPos);
		    
        	    // ���� ������ � ������ ������� - ������� ��� ��������������
        	    if ( (put.addr & (SPI_FLASH_SEC_SIZE-1)) == 0 )
        	    {
            		DEBUG("HTTPd: erase flash 0x%02X\n", (unsigned int)((put.addr & 0x7fffffff) / SPI_FLASH_SEC_SIZE));
            		spi_flash_erase_sector((put.addr & 0x7fffffff) / SPI_FLASH_SEC_SIZE);
        	    }
		    
        	    // ����������
        	    DEBUG("HTTPd: write flash 0x%X size=%d\n", (unsigned int)(put.addr & 0x7fffffff), put.bufferPos);
        	    spi_flash_write(put.addr & 0x7fffffff, (uint32*)put.buffer, PUT_BUFFER_SIZE);
		    
		    // ������� ��, ��� ��������
		    uint8_t *tmp=(uint8_t*)os_malloc(PUT_BUFFER_SIZE);
		    if (tmp) spi_flash_read(put.addr & 0x7fffffff, (uint32*)tmp, PUT_BUFFER_SIZE);
		    if ( (!tmp) || (os_memcmp(put.buffer, tmp, PUT_BUFFER_SIZE) != 0) )
		    {
			DEBUG("HTTPd: flash verify failed !!!\n");
			if (tmp) os_free((void*)tmp);
			
			static const char *str=
			    "HTTP/1.0 500 Internal server error\r\n"
			    "Content-Type: text/html\r\n"
			    "Connection: close\r\n"
			    "\r\n"
			    "Flash verify failed\r\n";
    			send((const uint8_t*)str, os_strlen(str));
    			close();
    			return;
		    }
		    os_free((void*)tmp);
		    
        	    put.addr+=PUT_BUFFER_SIZE;
        	    put.bufferPos=0;
    		}
	    }
	} else
	{
	    // ������ � ���
	    os_memcpy((uint8_t*)put.addr, data, len);
	    put.addr+=len;
	    put.size-=len;
	}
	
	if (put.size <= 0)
	{
	    // ����� ������
	    const char *answer=handler->webPutEnd(put.path);
	    if (! answer)
	    {
		static const char *str=
		    "HTTP/1.0 201 Created\r\n"
		    "Content-Type: text/html\r\n"
		    "Connection: close\r\n"
		    "\r\n"
		    "File created\r\n";
    		send((const uint8_t*)str, os_strlen(str));
    	    } else
    	    {
    		sendAndFree((const uint8_t*)answer, os_strlen(answer));
    	    }
    	    close();
    	    return;
	}
	
	return;
    } else
    if (http_method==METHOD_GET)
    {
	// � GET �� ����� ���� ������
	return;
    }
    
    goto again;
}


void HTTPd::dataSent()
{
    TCPSocket::dataSent();
    
    if (http_method==METHOD_GET)
    {
	if ( (get.addr==0) || (get.size<=0) )
	{
	    // ��� ������
            close();
            http_method=0;
            return;
	}
	
#define MAX_SEND_SIZE	((int)sizeof(buf))
        int l=get.size;
        if (l > MAX_SEND_SIZE) l=MAX_SEND_SIZE;
	
	if (get.addr & 0x80000000)
	{
    	    // ������ flash
    	    int l32=(l+3) & ~3;
    	    spi_flash_read(get.addr & 0x7fffffff, (uint32*)buf, l32);
    	} else
    	{
    	    // ������ ������
    	    os_memcpy(buf, (void*)(get.addr & 0x7fffffff), l);
    	}
	
        // ����������
        if (send((const uint8_t*)buf, l))
        {
    	    get.addr+=l;
    	    get.size-=l;
        }
	
        if (get.size <= 0)
        {
            // ����� ������
            close();
            http_method=0;
        }
    }
}


bool HTTPd::httpFS(const char *path)
{
    // �������� �� ������
    if ( (path[0]=='/') && (path[1]==0) ) path="/index.html";
    
    for (uint8_t i=0; i<64; i++)
    {
        struct ent
        {
            char name[24];
            int len;
            int offset;
        } e;
        spi_flash_read(FLASH_HTTP_DATA + i*sizeof(struct ent), (uint32*)&e, sizeof(struct ent));
        if (! e.name[0]) break; // ����� ������
        //DEBUG("HTTPd: fs '%s'\n", e.name);
        if (! os_strcmp(path, e.name))
        {
            // �����
            DEBUG("HTTPD: found '%s' size=%d offset=0x%x\n", e.name, e.len, e.offset);
            get.addr=(FLASH_HTTP_DATA + e.offset) | 0x80000000;
            get.size=e.len;
            
            // ����� ���������� ������ ��������
            dataSent();
            return true;
        }
    }

    // �� �����
    return false;
}
