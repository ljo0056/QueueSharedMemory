#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>

#include "QueueSharedMemory.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("no input queue name...");
        return 0;
    }

    std::string name = argv[1];
    std::string mode = argv[2];

    CQueueSharedMemory queue;
    int ret = queue.Initialize(name, 1024);
    if (ret)
    {
        printf("queue initialize failed   code[%d]\n", ret);
        return 0;
    }

    printf("Start name[%s]  Mode[%s]  QueueSize[%d]\n", name.c_str(), mode.c_str(), queue.GetQueueSize());

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