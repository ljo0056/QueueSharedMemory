#include "QueueSharedMemory.h"

#ifndef _WINDOWS_
#include <windows.h>
#endif


//////////////////////////////////////////////////////////////////////////


class CQueueSharedMemory::CQueueSharedMemoryImpl
{
private:
#pragma pack(push, 1)
    // Shared Memory �󿡼� ���� �Ǵ� Queue �� ����
    struct QueueInfo
    {
        uint32_t    tail;
        uint32_t    head;
        uint32_t    queue_size;
        uint32_t    use_size;         // ���� Queue �� ����� Data�� Byte ������

        // ���� : m_user_space ������ ����ü �������� ����Ǿ� �־�� �Ѵ�.
        // Shared Memory �� ������� Ŀ���� ������ �б�/���� �� �� �ִ� �����̴�.
        uint8_t     m_user_space[32];
    };
#pragma pack(pop)

    std::string    m_name;

    HANDLE         m_memory_map;
    QueueInfo*     m_queue_info;
    uint8_t*       m_queue_buffer;

    uint32_t       m_pop_data_len;
    uint32_t       m_error_code;

private:
    bool CreateSharedMemory(int queue_size)
    {
        if (0 == queue_size)
            return false;

        m_memory_map = CreateFileMappingA(
            INVALID_HANDLE_VALUE,               // hFile
            NULL,                               // lpFileMappingAttributes
            PAGE_READWRITE,                     // flProtect
            0,                                  // dwMaximumSizeHigh
            sizeof(QueueInfo) + queue_size,     // dwMaximumSizeLow
            m_name.c_str());                    // lpName

        if (NULL == m_memory_map)
        {
            m_error_code = GetLastError();
            return false;
        }

        return true;
    }

    bool OpenSharedMemory()
    {
        m_memory_map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_name.c_str());
        if (!m_memory_map)
        {
            m_error_code = GetLastError();
            return false;
        }

        return true;
    }

    bool GetSharedPoint()
    {
        m_queue_info = (QueueInfo*)MapViewOfFile(m_memory_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (nullptr == m_queue_info)
        {
            m_error_code = GetLastError();
            return false;
        }

        // ���� Queue�� ��ġ�� m_queue_info->m_user_space ��ġ ������ �ִ�.
        m_queue_buffer = reinterpret_cast<uint8_t*>(&m_queue_info->m_user_space[0]) + sizeof(m_queue_info->m_user_space);
        if (nullptr == m_queue_buffer)
            return false;

        return true;
    }

public:

    CQueueSharedMemoryImpl()
        : m_queue_buffer(nullptr)
        , m_queue_info(nullptr)
        , m_memory_map(NULL)
        , m_pop_data_len(0)
        , m_error_code(0)
    {

    }

    virtual ~CQueueSharedMemoryImpl()
    {
        Finalize();
    }

    int Initialize(const std::string& name, uint32_t queue_size)
    {
        m_name = name;

        bool create = false;
        if (false == OpenSharedMemory())
        {
            if (false == CreateSharedMemory(queue_size))
                return CREATE_MAMORY_MAP_HANDLE;

            create = true;
        }

        if (false == GetSharedPoint())
            return BRING_QUEUE_INFO;

        if (create)
            m_queue_info->queue_size = queue_size;

        return 0;
    }

    void Finalize()
    {
        if (m_queue_info)
        {
            UnmapViewOfFile(m_queue_info);
            m_queue_info = nullptr;
            m_queue_buffer = nullptr;
        }

        if (m_memory_map)
        {
            CloseHandle(m_memory_map);
            m_memory_map = NULL;
        }
    }

    std::string GetName() const
    {
        if (!m_queue_info)
            return std::string();

        return m_name;
    }

    int GetInformation(uint8_t* buffer)
    {
        if (!m_queue_info)
            return DID_NOT_INITIALIZE;

        memcpy(buffer, &m_queue_info->m_user_space[0], sizeof(m_queue_info->m_user_space));

        return 0;
    }

    int SetInformation(uint8_t* buffer)
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;

        memcpy(&m_queue_info->m_user_space[0], buffer, sizeof(m_queue_info->m_user_space));

        return 0;
    }

    int Clear()
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;

        ZeroMemory(m_queue_buffer, sizeof(uint8_t) * GetQueueSize());
        m_queue_info->head = 0;
        m_queue_info->tail = 0;
        m_queue_info->use_size = 0;

        return 0;
    }

    int Push(uint8_t* buffer, uint32_t buffer_len)
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;

        if (buffer_len > GetFreeSize())
            return NOT_ENOUGH_FREE_SPACE;

        // --- : ������ ����  *** : ������ ����
        //   0                                      queue_size
        //   |--------------H************T--------------|
        //                               |----------| <=== write_size
        uint32_t write_size = GetQueueSize() - m_queue_info->tail;
        if (write_size < buffer_len)
        {
            //   0                                      queue_size
            //   |--------------H************T--------------|
            //                               |--------------| <=== write_size Push
            // Tail �ڿ� buffer �� write_size ũ�� ��ŭ ���� �Ѵ�.
            memcpy(&m_queue_buffer[m_queue_info->tail], buffer, write_size);

            //   0                                      queue_size
            //   |--------------H************T--------------|
            //   |-----------| <=== (buffer_len - write_size) Push
            // ���� �����͸� ���� �Ѵ�.
            memcpy(&m_queue_buffer[0], &buffer[write_size], buffer_len - write_size);
            m_queue_info->tail = buffer_len - write_size;
        }
        else
        {
            //   0                                      queue_size
            //   |--------------H************T--------------|
            //                               |----------|  <== buffer_len Push
            memcpy(&m_queue_buffer[m_queue_info->tail], buffer, buffer_len);
            m_queue_info->tail += buffer_len;
        }

        m_queue_info->use_size += buffer_len;

        return 0;
    }

    int Front(uint8_t* buffer, uint32_t buffer_len)
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;

        if (m_queue_info->use_size < buffer_len)
            return READ_BUFFER_SIZE_IS_BIG;

        // --- : ������ ����  *** : ������ ����
        //   0                                      queue_size
        //   |**************T------------H**************|
        //                               |----------| <=== read_size
        uint32_t read_size = GetQueueSize() - m_queue_info->head;
        if (read_size < buffer_len)
        {
            //   0                                      queue_size
            //   |**************T------------H**************|
            //                               |--------------| <=== read_size Pop
            memcpy(buffer, &m_queue_buffer[m_queue_info->head], read_size);
            //   0                                      queue_size
            //   |**************T------------H**************|
            //   |--------| <=== (buffer_len - read_size) Pop
            memcpy(&buffer[read_size], &m_queue_buffer[0], buffer_len - read_size);
        }
        else
        {
            //   0                                      queue_size
            //   |--------------H************T--------------|
            //                  |----------| <=== buffer_len Pop
            memcpy(buffer, &m_queue_buffer[m_queue_info->head], buffer_len);
        }

        m_pop_data_len = buffer_len;

        return 0;
    }

    int Pop()
    {
        if (0 == m_pop_data_len)
            return POP_DATA_EMPTY;

        uint32_t nReadSize = GetQueueSize() - m_queue_info->head;
        if (nReadSize < m_pop_data_len)
            m_queue_info->head = m_pop_data_len - nReadSize;
        else
            m_queue_info->head = m_queue_info->head + m_pop_data_len;

        m_queue_info->use_size -= m_pop_data_len;

        m_pop_data_len = 0;

        return 0;
    }

    int SetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len)
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;
        if (pos < 0 || pos + buffer_len >= GetQueueSize())
            return RANGE_IS_NOT_RIGHT;

        memcpy(&m_queue_buffer[pos], buffer, buffer_len);

        return 0;
    }

    int GetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len)
    {
        if (nullptr == m_queue_info)
            return DID_NOT_INITIALIZE;
        if (pos < 0 || pos >= GetQueueSize())
            return RANGE_IS_NOT_RIGHT;
        if (GetQueueSize() - pos < buffer_len)
            return RANGE_IS_NOT_RIGHT;

        memcpy(buffer, &m_queue_buffer[pos], buffer_len);

        return 0;
    }

    uint32_t GetUseSize() const
    {
        if (!m_queue_info)
            return 0;

        return m_queue_info->use_size;
    }

    uint32_t GetQueueSize() const
    {
        if (!m_queue_info)
            return 0;

        return m_queue_info->queue_size;
    }

    uint32_t GetFreeSize() const
    {
        if (!m_queue_info)
            return 0;

        return (m_queue_info->queue_size - sizeof(uint8_t)) - m_queue_info->use_size;
    }

    uint32_t GetWinErrorCode() const
    {
        return m_error_code;
    }
};

//////////////////////////////////////////////////////////////////////////

CQueueSharedMemory::CQueueSharedMemory()
    : m_impl(new CQueueSharedMemoryImpl)
{

}

CQueueSharedMemory::~CQueueSharedMemory()
{
    Finalize();
}

int CQueueSharedMemory::Initialize(const std::string& name, uint32_t queue_size)
{
    return m_impl->Initialize(name, queue_size);
}

void CQueueSharedMemory::Finalize()
{
    m_impl->Finalize();
}

std::string CQueueSharedMemory::GetName() const
{
    return m_impl->GetName();
}

int CQueueSharedMemory::GetInformation(uint8_t* buffer)
{
    return m_impl->GetInformation(buffer);
}

int CQueueSharedMemory::SetInformation(uint8_t* buffer)
{
    return m_impl->SetInformation(buffer);
}

int CQueueSharedMemory::Clear()
{
    return m_impl->Clear();
}

int CQueueSharedMemory::Push(uint8_t* buffer, uint32_t buffer_len)
{
    return m_impl->Push(buffer, buffer_len);
}

int CQueueSharedMemory::Front(uint8_t* buffer, uint32_t buffer_len)
{
    return m_impl->Front(buffer, buffer_len);
}

int CQueueSharedMemory::Pop()
{
    return m_impl->Pop();
}

int CQueueSharedMemory::SetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len)
{
    return m_impl->SetData(pos, buffer, buffer_len);
}

int CQueueSharedMemory::GetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len)
{
    return m_impl->GetData(pos, buffer, buffer_len);
}

uint32_t CQueueSharedMemory::GetUseSize() const
{
    return m_impl->GetUseSize();
}

uint32_t CQueueSharedMemory::GetQueueSize() const
{
    return m_impl->GetQueueSize();
}

uint32_t CQueueSharedMemory::GetFreeSize() const
{
    return m_impl->GetFreeSize();
}

uint32_t CQueueSharedMemory::GetWinErrorCode() const
{
    return m_impl->GetWinErrorCode();
}


//////////////////////////////////////////////////////////////////////////
// Test Code

int TestQueueSharedMemory()
{
    std::string name = "MyFileMappingObject";
    std::string str_send;
    uint8_t buffer[32] = { 0, };

    // �ʱ�ȭ
    CQueueSharedMemory queue1;
    if (queue1.Initialize(name, 128))
        return 1;

    CQueueSharedMemory queue2;
    if (queue2.Initialize(name, 0))
        return 2;

    // Write
    str_send = "hello world";
    memcpy(buffer, str_send.c_str(), str_send.size());
    queue1.Push(buffer, (uint32_t)str_send.size());

    // Read
    std::string str_recv;
    queue2.Front(buffer, 11);
    queue2.Pop();
    str_recv = (char*)buffer;

    // ����
    if (str_send != str_recv)
        return 3;

    return 0;
}

#if 0   // Process use sample code

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>

#include "QueueSharedMemory.h"

// server excute : QueueSharedMemory.exe MySharedMemory server
// client excute : QueueSharedMemory.exe MySharedMemory client

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("no input queue name...");
        return 0;
    }

    std::string name = argv[1];
    std::string mode = argv[2];

    printf("Start name[%s]  Mode[%s]\n", name.c_str(), mode.c_str());

    CQueueSharedMemory queue;
    int ret = queue.Initialize(name, 1024);
    if (ret)
    {
        printf("queue initialize failed   code[%d]\n", ret);
        return 0;
    }

    std::string message;

    while (true)
    {
        if ("server" == mode)
        {
            uint32_t use_size = queue.GetUseSize();
            if (0 == use_size)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            static uint8_t buffer[64] = { 0, };
            memset(buffer, 0, sizeof(buffer));

            queue.Front(buffer, use_size);
            queue.Pop();

            message = (char*)buffer;
            printf("recv  message [%s] len[%d]\n", message.c_str(), message.size());
        }
        else if ("client" == mode)
        {
            printf("input message : ");
            std::getline(std::cin, message);

            queue.Push((uint8_t*)message.c_str(), (uint32_t)message.size());
            printf("send  message [%s] len[%d]\n", message.c_str(), message.size());
        }

        if ("quit" == message)
            break;
    }

    return 0;
}
#endif