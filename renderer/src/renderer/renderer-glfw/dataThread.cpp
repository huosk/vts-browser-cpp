#include <unistd.h> // usleep

#include <GLFW/glfw3.h>

#include "dataThread.h"
#include <renderer/map.h>

#include "threadName.h"

namespace
{
    void run(DataThread *data)
    {
        data->run();
    }
}

DataThread::DataThread(GLFWwindow *shared) : window(nullptr), map(nullptr), stop(false)
{
    fetcher = std::shared_ptr<Fetcher>(Fetcher::create());
    window = glfwCreateWindow(1, 1, "data context", NULL, shared);
    glfwSetWindowUserPointer(window, this);
    glfwHideWindow(window);
    gpu.initialize();
    thr = std::thread(&::run, this);
}

DataThread::~DataThread()
{
    stop = true;
    thr.join();
    glfwDestroyWindow(window);
    window = nullptr;
}

void DataThread::run()
{
    setThreadName("renderer-data");
    glfwMakeContextCurrent(window);
    while (!stop && !map)
        usleep(1000);
    map->dataInitialize(&gpu, fetcher.get());
    while (!stop)
    {
        fetcher->tick();
        if (map->dataTick())
            usleep(1000);
    }
    map->dataFinalize();
}