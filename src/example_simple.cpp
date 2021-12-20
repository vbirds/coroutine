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
#include <queue>
#include "co_routine.h"

void* test(void* args)
{
	co_enable_hook_sys();

    int task_id = *((int*)args);
    for (int i = 0; i < 10; i++)
    {
        printf("task id %d loop %d\n", task_id, i);
        co_yield_ct_timeout(1000);
    }
    printf("end task id %d\n", task_id);
	return NULL;
}
int main()
{
    co_start_hook();
    for (int i = 0; i < 3; i++)
    {
        printf("begin task id %d\n", i);
        stCoRoutine_t* routine;
        co_create(&routine, NULL, test, &i);
        co_resume(routine);
    }

	co_eventloop(co_get_epoll_ct(), NULL, NULL);
	return 0;
}
