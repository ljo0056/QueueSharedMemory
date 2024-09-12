#pragma once

//////////////////////////////////////////////////////////////////////////
///  @file    QueueSharedMemory.h
///  @date    2019/06/13
///  @author  Lee Jong Oh
///

//////////////////////////////////////////////////////////////////////////
///  @class   CQueueSharedMemory
///  @brief   ���μ����� ���� �Ҽ� �ִ� ���� Queue �� ���� �Ѵ�.
///           ������ Queue �� �޸𸮿� Read / Write �� ���Ͽ� ���μ����� ����� �� �� �ִ�.

#include <memory>
#include <string>

class CQueueSharedMemory
{
private:
    class CQueueSharedMemoryImpl;
    std::unique_ptr<CQueueSharedMemoryImpl> m_impl;

public:
    enum FailedCode
    {
        CREATE_MAMORY_MAP_HANDLE = 1,   // Shared Memory ������ ����  GetWinErrorCode() �� ���� code �� Ȯ�� �� �� �ִ�.
        BRING_QUEUE_INFO,               // Queue ������ �������µ� ����
        DID_NOT_INITIALIZE,             // �ʱ�ȭ�� �������� �ʾ���
        NOT_ENOUGH_FREE_SPACE,          // Queue �� ���� ������ ����
        READ_BUFFER_SIZE_IS_BIG,        // Read �ϰ��� �ϴ� buffer ����� Queue �� ����� Use size ���� ŭ
        POP_DATA_EMPTY,                 // Pop �� �����Ͱ� ����
        RANGE_IS_NOT_RIGHT,             // ������ ���� ����
    };

    CQueueSharedMemory();
    virtual ~CQueueSharedMemory();

    ///  @brief      Shared Memory ����� ���� ��ü�� �Ҵ��ϰ� �ʱ�ȭ ��Ų��.
    ///              MSDN : CreateFileMapping, OpenFileMapping, MapViewOfFile API ����
    ///  @param name[in] : Shared Memory �� �̸��� �����Ͽ� �̸��� Key �� �ܺο� ���� �Ѵ�.
    ///                    name �� Global namespace �� ��������� ������� �Ѵٸ� ������ ������ �ʿ� �ϴ�.
    ///                    �ڼ��� ������ MSDN �� ����
    ///  @param queue_size[in] : Byte ������ Queue size �� ����
    ///                          �ش� Name ���� �̹� Shared Memory �� �ִٸ� queue_size �� ���� �ǰ�
    ///                          ���� ������� ������ ���� �Ѵ�.
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int  Initialize(const std::string& name, uint32_t queue_size);

    ///  @brief      Shared Memory ����� ���� ������ ��ü���� ���� �Ѵ�.
    void Finalize();

    ///  @brief      Initialize �Լ��� name �Ķ���͸� return �Ѵ�.
    ///  @return     ���� �ÿ� Shared Memory �� name �� return, ���нÿ� �� ���ڿ� return
    std::string GetName() const;

    ///  @brief      Shared Memory �� ����� 32����Ʈ ũ���� ����� �����͸� ��´�.
    ///  @param buffer[out] : 32 ����Ʈ�� �Ҵ�� buffer �� ����� �����͸� copy �Ͽ� ��ȯ�Ѵ�.
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int GetInformation(uint8_t* buffer);

    ///  @brief      Shared Memory �� ����� 32����Ʈ ũ���� ����� �����͸� ���� �Ѵ�.
    ///  @param buffer[in] : buffer �� �����͸� 32����Ʈ ��ŭ Shared Memory �� copy �Ѵ�.
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int SetInformation(uint8_t* buffer);

    ///  @brief      Shared Memory �� Queue �� ����.
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int Clear();

    ///  @brief      Shared Memory �� Queue �� �����͸� �ڿ� �߰� �Ѵ�.
    ///  @param buffer[in] : buffer �� �����͸� buffer_len ���� ��ŭ Queue �� copy �Ѵ�.
    ///  @param buffer_len[in] : buffer �� ������ ���� (Byte)
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int Push(uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory �� Queue ���� ���� ���� �����͸� �����´�.
    ///              ���� ���Ŀ� Pop() �Լ��� ȣ���ؾ� Queue ���� �����Ͱ� ���� �ȴ�.
    ///  @param buffer[out] : buffer �� �����͸� buffer_len ���� ��ŭ Queue ���� copy �Ѵ�.
    ///  @param buffer_len[in] : buffer �� ������ ���� (Byte)
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int Front(uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Front() �Լ� ȣ�� �Ŀ� Queue ���� �����͸� ���� �Ѵ�.    
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int Pop();

    ///  @brief      Shared Memory Queue �� Ư���� ��ġ�� �����͸� copy �Ѵ�.
    ///  @param pos[in] : Queue �� ��ġ
    ///  @param buffer[in] : buffer �� �����͸� buffer_len ���� ��ŭ Queue �� copy �Ѵ�.
    ///  @param buffer_len[in] : buffer �� ������ ���� (Byte)
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int SetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory Queue �� Ư���� ��ġ�� �����͸� buffer �� copy �Ѵ�.
    ///  @param pos[in] : Queue �� ��ġ
    ///  @param buffer[out] : buffer �� �����͸� buffer_len ���� ��ŭ Queue ���� copy �Ѵ�.
    ///  @param buffer_len[in] : buffer �� ������ ���� (Byte)
    ///  @return     ���� �ÿ� 0, ���� �ÿ� FailedCode �� return �Ѵ�.
    int GetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory Queue �� ������ ������ Byte ũ�⸦ return �Ѵ�.
    ///  @return     ���� �ÿ� Queue �� ���� Byte ũ��, ���� �ÿ� 0 �� return �Ѵ�.
    uint32_t GetUseSize() const;

    ///  @brief      Shared Memory Queue �� ���� ũ�⸦ return �Ѵ�.
    ///  @return     ���� �ÿ� Queue �� Byte ũ��, ���� �ÿ� 0 �� return �Ѵ�.
    uint32_t GetQueueSize() const;

    ///  @brief      Shared Memory Queue �� ������ ������ Byte ���� ũ��� return �Ѵ�.
    ///  @return     ���� �ÿ� Queue �� ���� Byte ũ��, ���� �ÿ� 0 �� return �Ѵ�.
    uint32_t GetFreeSize() const;

    ///  @brief      Windows API ȣ�� �� ���� �ÿ� GetLastError() �� �ڵ� ���� return �Ѵ�.
    ///  @return     GetLastError() �ڵ尪�� return �Ѵ�.
    uint32_t GetWinErrorCode() const;
};


//////////////////////////////////////////////////////////////////////////
// Test Code

int TestQueueSharedMemory();