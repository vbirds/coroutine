/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "co_routine.h"

#include <vector>

std::vector<stCoRoutine_t*> g_coRoutineVec;


void* test(void* args)
{
	co_enable_hook_sys();
    // 测试co 递归调用链
    if (g_coRoutineVec.empty())
    {
        printf("last hello %d\n", *((int*)args));
        return NULL;
    }

    stCoRoutine_t *co = *g_coRoutineVec.begin();
    g_coRoutineVec.erase(g_coRoutineVec.begin());
    co_resume(co);
    printf("hello %d\n", *((int*)args));
    // delete args;
	return NULL;
}

int main()
{
    co_start_hook();

    // 测试libco 调用链最多递归调用128层
    stCoRoutine_t * first_coroutine = nullptr;
    for (int i = 0; i < 127; i++)
    {
        int *args = new int;
        *args = i;
        stCoRoutine_t* routine;
        co_create(&routine, NULL, test, args);

        if (i == 0)
        {
            first_coroutine = routine;
        }
        else
        {
            g_coRoutineVec.push_back(routine);
        }
    }

    co_resume(first_coroutine);
    printf("test\n");
	co_eventloop(co_get_epoll_ct(), NULL, NULL);
	return 0;
}
