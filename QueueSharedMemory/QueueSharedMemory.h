#pragma once

//////////////////////////////////////////////////////////////////////////
///  @file    QueueSharedMemory.h
///  @date    2019/06/13
///  @author  Lee Jong Oh
///

//////////////////////////////////////////////////////////////////////////
///  @class   CQueueSharedMemory
///  @brief   프로세스간 공유 할수 있는 원형 Queue 를 생성 한다.
///           생성된 Queue 는 메모리에 Read / Write 를 통하여 프로세스간 통신을 할 수 있다.

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
        CREATE_MAMORY_MAP_HANDLE = 1,   // Shared Memory 생성에 실패  GetWinErrorCode() 를 통해 code 를 확인 할 수 있다.
        BRING_QUEUE_INFO,               // Queue 정보를 가져오는데 실패
        DID_NOT_INITIALIZE,             // 초기화를 수행하지 않았음
        NOT_ENOUGH_FREE_SPACE,          // Queue 의 여유 공간이 적음
        READ_BUFFER_SIZE_IS_BIG,        // Read 하고자 하는 buffer 사이즈가 Queue 에 적재된 Use size 보다 큼
        POP_DATA_EMPTY,                 // Pop 할 데이터가 없음
        RANGE_IS_NOT_RIGHT,             // 범위가 맞지 않음
    };

    CQueueSharedMemory();
    virtual ~CQueueSharedMemory();

    ///  @brief      Shared Memory 사용을 위해 객체를 할당하고 초기화 시킨다.
    ///              MSDN : CreateFileMapping, OpenFileMapping, MapViewOfFile API 참조
    ///  @param name[in] : Shared Memory 에 이름을 설정하여 이름을 Key 로 외부와 공유 한다.
    ///                    name 이 Global namespace 를 명시적으로 만들고자 한다면 관리자 권한이 필요 하다.
    ///                    자세한 내용은 MSDN 을 참조
    ///  @param queue_size[in] : Byte 단위의 Queue size 를 설정
    ///                          해당 Name 으로 이미 Shared Memory 가 있다면 queue_size 는 무시 되고
    ///                          최초 만들었던 정보로 설정 한다.
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int  Initialize(const std::string& name, uint32_t queue_size);

    ///  @brief      Shared Memory 사용을 위해 생성된 객체들을 정리 한다.
    void Finalize();

    ///  @brief      Initialize 함수의 name 파라미터를 return 한다.
    ///  @return     성공 시에 Shared Memory 의 name 을 return, 실패시에 빈 문자열 return
    std::string GetName() const;

    ///  @brief      Shared Memory 에 저장된 32바이트 크기의 사용자 데이터를 얻는다.
    ///  @param buffer[out] : 32 바이트가 할당된 buffer 에 사용자 데이터를 copy 하여 반환한다.
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int GetInformation(uint8_t* buffer);

    ///  @brief      Shared Memory 에 저장된 32바이트 크기의 사용자 데이터를 저장 한다.
    ///  @param buffer[in] : buffer 의 데이터를 32바이트 만큼 Shared Memory 에 copy 한다.
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int SetInformation(uint8_t* buffer);

    ///  @brief      Shared Memory 의 Queue 를 비운다.
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int Clear();

    ///  @brief      Shared Memory 의 Queue 에 데이터를 뒤에 추가 한다.
    ///  @param buffer[in] : buffer 의 데이터를 buffer_len 길이 만큼 Queue 에 copy 한다.
    ///  @param buffer_len[in] : buffer 의 데이터 길이 (Byte)
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int Push(uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory 의 Queue 에서 제일 앞의 데이터를 가져온다.
    ///              가져 온후에 Pop() 함수를 호출해야 Queue 에서 데이터가 삭제 된다.
    ///  @param buffer[out] : buffer 의 데이터를 buffer_len 길이 만큼 Queue 에서 copy 한다.
    ///  @param buffer_len[in] : buffer 의 데이터 길이 (Byte)
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int Front(uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Front() 함수 호출 후에 Queue 에서 데이터를 삭제 한다.    
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int Pop();

    ///  @brief      Shared Memory Queue 에 특정한 위치에 데이터를 copy 한다.
    ///  @param pos[in] : Queue 의 위치
    ///  @param buffer[in] : buffer 의 데이터를 buffer_len 길이 만큼 Queue 에 copy 한다.
    ///  @param buffer_len[in] : buffer 의 데이터 길이 (Byte)
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int SetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory Queue 에 특정한 위치에 데이터를 buffer 에 copy 한다.
    ///  @param pos[in] : Queue 의 위치
    ///  @param buffer[out] : buffer 의 데이터를 buffer_len 길이 만큼 Queue 에서 copy 한다.
    ///  @param buffer_len[in] : buffer 의 데이터 길이 (Byte)
    ///  @return     성공 시에 0, 실패 시에 FailedCode 을 return 한다.
    int GetData(uint32_t pos, uint8_t* buffer, uint32_t buffer_len);

    ///  @brief      Shared Memory Queue 에 적제된 데이터 Byte 크기를 return 한다.
    ///  @return     성공 시에 Queue 에 사용된 Byte 크기, 실패 시에 0 을 return 한다.
    uint32_t GetUseSize() const;

    ///  @brief      Shared Memory Queue 의 실제 크기를 return 한다.
    ///  @return     성공 시에 Queue 의 Byte 크기, 실패 시에 0 을 return 한다.
    uint32_t GetQueueSize() const;

    ///  @brief      Shared Memory Queue 에 여분의 공간을 Byte 단위 크기로 return 한다.
    ///  @return     성공 시에 Queue 의 여분 Byte 크기, 실패 시에 0 을 return 한다.
    uint32_t GetFreeSize() const;

    ///  @brief      Windows API 호출 후 실패 시에 GetLastError() 의 코드 값을 return 한다.
    ///  @return     GetLastError() 코드값을 return 한다.
    uint32_t GetWinErrorCode() const;
};


//////////////////////////////////////////////////////////////////////////
// Test Code

int TestQueueSharedMemory();