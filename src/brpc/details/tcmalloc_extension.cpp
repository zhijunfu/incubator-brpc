// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <pthread.h>
#include <dlfcn.h>                               // dlsym
#include <stdlib.h>                              // getenv
#include "butil/compiler_specific.h"
#include "butil/logging.h"
#include "brpc/details/tcmalloc_extension.h"

namespace {
typedef MallocExtension* (*GetInstanceFn)();
static pthread_once_t g_get_instance_fn_once = PTHREAD_ONCE_INIT;
static GetInstanceFn g_get_instance_fn = NULL;
static void InitGetInstanceFn() {
    g_get_instance_fn = (GetInstanceFn)dlsym(
        RTLD_NEXT, "_ZN15MallocExtension8instanceEv");
    if (!g_get_instance_fn) {
        // NOTE(zhijunfu): There are two constants that you can give to dlsym() instead of
        // a module name:
        // - RTLD_DEFAULT: search through every .so in the process in the order they
        //   were loaded.
        // - RTLD_NEXT: search through every .so that were loaded *after* the one calling
        //   dlsym(), so if you call dlsym() in your main program using RTLD_NEXT, it will
        //   look in any libraries loaded after your program started running. 
        //
        // For more cases RTLD_NEXT should be sufficient, but in some cases we need to
        // call dlsym() in a .so, then it might not work correctly. One such case is
        // that java worker loads libcore_worker_library_java.so in the java code,
        // and libtcmalloc.so is loaded by using LD_PRELOAD, in this case the module
        // that calls dlsym() is loaded *after* libtcmalloc.so, and using RTLD_NEXT
        // here would not find the symbol, and we try again with RTLD_DEFAULT in
        // this case.
        g_get_instance_fn = (GetInstanceFn)dlsym(
            RTLD_DEFAULT, "_ZN15MallocExtension8instanceEv");
        LOG(INFO) << "MallocExtension symbol is not found with RTLD_NEXT"
                  << ", and " << (g_get_instance_fn ? "is" : "not")
                  << " found with RTLD_DEFAULT";
    }
}
} // namespace

MallocExtension* BAIDU_WEAK MallocExtension::instance() {
    // On fedora 26, this weak function is NOT overriden by the one in tcmalloc
    // which is dynamically linked.The same issue can't be re-produced in
    // Ubuntu and the exact cause is unknown yet. Using dlsym to get the
    // function works around the issue right now. Note that we can't use dlsym
    // to fully replace the weak-function mechanism since our code are generally
    // not compiled with -rdynamic which writes symbols to the table that
    // dlsym reads.
    pthread_once(&g_get_instance_fn_once, InitGetInstanceFn);
    if (g_get_instance_fn) {
        return g_get_instance_fn();
    }
    return NULL;
}

bool IsHeapProfilerEnabled() {
    return MallocExtension::instance() != NULL;
}

static bool check_TCMALLOC_SAMPLE_PARAMETER() {
    char* str = getenv("TCMALLOC_SAMPLE_PARAMETER");
    if (str == NULL) {
        return false;
    }
    char* endptr;
    int val = strtol(str, &endptr, 10);
    return (*endptr == '\0' && val > 0);
}

bool has_TCMALLOC_SAMPLE_PARAMETER() {
    static bool val = check_TCMALLOC_SAMPLE_PARAMETER();
    return val;
}
