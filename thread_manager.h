#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <QObject>
#include <QThreadPool>
#include <QThread>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <functional>

/**
 * @brief 线程管理器 - 采用单例模式，提供全局线程池和异步任务调度
 */
class ThreadManager : public QObject
{
    Q_OBJECT
public:
    static ThreadManager* getInstance();
    
    // 在全局线程池中运行一个无返回值的异步任务
    void runAsync(std::function<void()> task);
    
    // 运行一个带返回值的异步任务
    template<typename T>
    QFuture<T> runAsyncWithResult(std::function<T()> task) {
        return QtConcurrent::run(m_threadPool, task);
    }

    // 获取全局线程池
    QThreadPool* getThreadPool() { return m_threadPool; }

private:
    explicit ThreadManager(QObject *parent = nullptr);
    static ThreadManager* m_instance;
    QThreadPool* m_threadPool;
};

#endif // THREAD_MANAGER_H
