#include "thread_manager.h"

ThreadManager* ThreadManager::m_instance = nullptr;

ThreadManager* ThreadManager::getInstance() {
    if (!m_instance) {
        m_instance = new ThreadManager();
    }
    return m_instance;
}

ThreadManager::ThreadManager(QObject *parent) : QObject(parent) {
    m_threadPool = new QThreadPool(this);
    // 根据 CPU 核心数设置最大线程数，通常为 核心数 * 2
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount() * 2);
}

void ThreadManager::runAsync(std::function<void()> task) {
    auto future = QtConcurrent::run(m_threadPool, task);
    Q_UNUSED(future);
}
