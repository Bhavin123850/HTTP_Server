#pragma once

#include "Common.h"



// ============================================================
// 7. TASK QUEUE
// ============================================================

class TaskQueue
{
private:

    queue<
        function<void()>
    > tasks;


    mutex mtx;


    condition_variable cv;


public:

    // ========================================================
    // PUSH TASK
    // ========================================================

    void push(
        function<void()> task
    )
    {
        {
            lock_guard<mutex> lock(mtx);


            tasks.push(
                move(task)
            );
        }


        cv.notify_one();
    }


    // ========================================================
    // POP TASK
    // ========================================================

    function<void()> pop()
    {
        unique_lock<mutex> lock(mtx);


        cv.wait(
            lock,
            [this]()
            {
                return !tasks.empty();
            }
        );


        function<void()> task =
            move(tasks.front());


        tasks.pop();


        return task;
    }
};



// ============================================================
// 8. THREAD POOL
// ============================================================

class ThreadPool
{
private:

    vector<thread> workers;


    TaskQueue taskQueue;


public:

    ThreadPool(
        int numberOfThreads
    )
    {
        for(
            int i = 0;
            i < numberOfThreads;
            i++
        )
        {
            workers.emplace_back(
                [this, i]()
                {
                    while(true)
                    {
                        function<void()> task =
                            taskQueue.pop();


                        cout
                            << "[Worker "
                            << i
                            << "] Thread ID = "
                            << this_thread::get_id()
                            << endl;


                        task();
                    }
                }
            );
        }
    }


    // ========================================================
    // SUBMIT TASK
    // ========================================================

    void submit(
        function<void()> task
    )
    {
        taskQueue.push(
            move(task)
        );
    }


    // ========================================================
    // DESTRUCTOR
    // ========================================================

    ~ThreadPool()
    {
        for(auto& worker : workers)
        {
            worker.detach();
        }
    }
};
