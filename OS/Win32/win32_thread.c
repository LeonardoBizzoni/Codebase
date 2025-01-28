fn OS_W32_Thread* 
os_w32_thread_alloc(void)
{
    OS_W32_Thread *result = w32_state.free_list;
    if(result)
    {
        StackPop(w32_state.free_list);
    }
    else
    {
        Assert(w32_state.pos < 256);
        result = &w32_state.thread_pool[w32_state.pos];
        w32_state.pos += 1;
    }
    memset(result, 0, sizeof(*result));
    return result;
}

fn void 
os_w32_thread_release(OS_W32_Thread *thread)
{
    StackPush(w32_state.free_list, thread);
}

fn OS_Thread 
os_thread_start(ThreadFunc *func, void *arg)
{
    OS_W32_Thread *thread = os_w32_thread_alloc();
    HANDLE handle = CreateThread(0, 0, os_w32_thread_entry_point, thread, 0, &thread->tid);
    thread->func = func;
    thread->arg = arg;
    thread->handle = handle;
    OS_Thread result = {(u64)thread};
    return result;
}

fn bool
os_thread_join(OS_Thread handle)
{
    OS_W32_Thread *thread = (OS_W32_Thread*)handle.v[0];
    DWORD wait = WAIT_OBJECT_0;
    if(thread)
    {
        wait = WaitForSingleObject(thread->handle, INFINITE);
        CloseHandle(thread->handle);
        os_w32_thread_release(thread);
    }
    return wait == WAIT_OBJECT_0;
}

fn DWORD 
os_w32_thread_entry_point(void *ptr)
{
    OS_W32_Thread *thread = (OS_W32_Thread*)ptr;
    ThreadFunc *func = thread->func;
    func(thread->arg);
    return 0;
}
