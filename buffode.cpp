#include "qtwraper.h"
#include <QDebug>
#include <QThread>
#include "mainwindow.h"

extern MainWindow* window;

// Function to get current time (you need to implement this)
void getTime(int64_t* time);

processArincMainFunction mainArincWraper;

int processArincMainFunction::createProcess(QString processName, void* _p_process) {
    qDebug() << "processArincMainFunction::createProcess " << processName;

    QtConcurrent::run([=]() {
        quint64 id = (quint64)QThread::currentThreadId();

        auto pinfo = std::make_shared<processInfo>(processName, id, _p_process);

        lock.lock();
        process.insert(id, pinfo);
        processOrder.append(id);
        lock.unlock();

        // Notify the main thread that a new process is ready
        {
            std::lock_guard<std::mutex> lk(mtx_main);
            pinfo->is_ready = true;
        }
        cv_main.notify_one();

        // Now, execute the function
        ((int(__cdecl *)())_p_process)();
    });

    return 0;
}

void processArincMainFunction::timeWait(int64_t _time) {
    quint64 id = (quint64)QThread::currentThreadId();
    lock.lock();
    auto it = process.find(id);
    if (it == process.end()) {
        lock.unlock();
        return;
    }
    auto pinfo = it.value();
    lock.unlock();

    qDebug() << "processArincMainFunction::timeWait" << _time << pinfo->name;

    std::unique_lock<std::mutex> lk(pinfo->mtx);
    pinfo->pause = _time;
    getTime(&(pinfo->timestop));
    pinfo->is_waiting = true;

    // Notify the main thread that this process is waiting
    cv_main.notify_one();

    pinfo->cv.wait(lk, [pinfo]() {
        return !pinfo->is_waiting.load();
    });
}

void processArincMainFunction::periodicWait(void) {
    quint64 id = (quint64)QThread::currentThreadId();
    lock.lock();
    auto it = process.find(id);
    if (it == process.end()) {
        lock.unlock();
        return;
    }
    auto pinfo = it.value();
    lock.unlock();

    qDebug() << "processArincMainFunction::periodicWait" << pinfo->name;

    std::unique_lock<std::mutex> lk(pinfo->mtx);
    pinfo->is_waiting = true;

    // Notify the main thread that this process is waiting
    cv_main.notify_one();

    pinfo->cv.wait(lk, [pinfo]() {
        return !pinfo->is_waiting.load();
    });
}

void processArincMainFunction::START_ALL_PROCESS(bool* autoMode, int sleepTime) {
    qDebug() << "processArincMainFunction::START_ALL_PROCESS";

    while (1) {
        // Wait until at least one process is ready or waiting
        {
            std::unique_lock<std::mutex> lk(mtx_main);
            cv_main.wait(lk, [this]() {
                // Check if any process is ready or waiting
                lock.lock();
                for (const auto& id : processOrder) {
                    auto pinfo = process.value(id);
                    if (pinfo->is_ready || pinfo->is_waiting) {
                        lock.unlock();
                        return true;
                    }
                }
                lock.unlock();
                return false;
            });
        }

        lock.lock();
        auto processesSnapshot = processOrder;
        lock.unlock();

        for (const auto& id : processesSnapshot) {
            lock.lock();
            auto it = process.find(id);
            if (it == process.end()) {
                lock.unlock();
                continue;
            }
            auto pinfo = it.value();
            lock.unlock();

            qDebug() << "Checking process:" << pinfo->name;

            // If the process is newly created and ready, we need to wait until it starts waiting
            if (pinfo->is_ready) {
                std::unique_lock<std::mutex> lk(pinfo->mtx);
                pinfo->is_ready = false;
                // Wait until the process sets is_waiting to true
                pinfo->cv.wait(lk, [pinfo]() {
                    return pinfo->is_waiting.load();
                });
            }

            // If the process is waiting, check if it's time to wake it up
            if (pinfo->is_waiting) {
                std::unique_lock<std::mutex> lk(pinfo->mtx);
                int64_t tnow;
                getTime(&tnow);
                if (tnow - pinfo->timestop >= pinfo->pause) {
                    // Time to wake up the process
                    pinfo->is_waiting = false;
                    pinfo->cv.notify_one();
                } else {
                    // Not yet time, continue to next process
                    continue;
                }
            }
        }

        // Update UI components
        QMetaObject::invokeMethod(window, []() {
            window->updateWindowsMemoryBlock();
            window->updateWindowsPorts();
        });

        if (!*autoMode)
            return;

        // Since we cannot use sleep, we'll wait for a condition variable with a timeout
        {
            std::unique_lock<std::mutex> lk(mtx_main);
            cv_main.wait_for(lk, std::chrono::milliseconds(sleepTime));
        }
    }
}


#ifndef QTWRAPER_H
#define QTWRAPER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QThread>
#include <QtConcurrent>
#include <condition_variable>
#include <atomic>
#include <memory>

struct processInfo {
    QString name;
    quint64 idthread;
    void* func;
    int64_t timestop = 0; // time of stop
    int64_t pause = 0;    // pause duration
    std::condition_variable cv;
    std::mutex mtx;
    std::atomic<bool> is_waiting{false};
    std::atomic<bool> is_ready{false};

    processInfo(QString name, quint64 idthread, void* func)
        : name(name), idthread(idthread), func(func) {}

    // Delete copy constructor and assignment operator
    processInfo(const processInfo&) = delete;
    processInfo& operator=(const processInfo&) = delete;
};

class processArincMainFunction {
    // PROCESS
    QMap<quint64, std::shared_ptr<processInfo>> process;
    QVector<quint64> processOrder;

    // MEMORYBLOCK
    QMap<QString, QByteArray> memoryBlockData;

    // QUEUING_PORT
    QVector<QByteArray> portQueuings;
    QMap<QString, int> idPortQueuings;

    QMutex lock;
    std::condition_variable cv_main;
    std::mutex mtx_main;

public:
    int createProcess(QString processName, void* _p_process);
    void timeWait(int64_t _time);
    void periodicWait(void);
    void START_ALL_PROCESS(bool* autoMode, int sleepTime);
};

extern processArincMainFunction mainArincWraper;

#endif // QTWRAPER_H



#ifndef QTWRAPER_H
#define QTWRAPER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QMutex>
#include <QThread>
#include <QtConcurrent>
#include <condition_variable>
#include <atomic>
#include <memory>

struct processInfo {
    QString name;
    quint64 idthread;
    void* func;
    int64_t timestop = 0; // time of stop
    int64_t pause = 0;    // pause duration
    std::condition_variable cv;
    std::mutex mtx;
    std::atomic<bool> is_waiting{false};

    processInfo(QString name, quint64 idthread, void* func)
        : name(name), idthread(idthread), func(func) {}

    // Delete copy constructor and assignment operator
    processInfo(const processInfo&) = delete;
    processInfo& operator=(const processInfo&) = delete;
};

class processArincMainFunction {
    // PROCESS
    QMap<quint64, std::shared_ptr<processInfo>> process;
    QVector<quint64> processOrder;

    // MEMORYBLOCK
    QMap<QString, QByteArray> memoryBlockData;

    // QUEUING_PORT
    QVector<QByteArray> portQueuings;
    QMap<QString, int> idPortQueuings;

    QMutex lock;

public:
    int createProcess(QString processName, void* _p_process);
    void timeWait(int64_t _time);
    void periodicWait(void);
    void START_ALL_PROCESS(bool* autoMode, int sleepTime);
};

extern processArincMainFunction mainArincWraper;

#endif // QTWRAPER_H




#include "qtwraper.h"
#include <QDebug>
#include <QThread>
#include "mainwindow.h"

extern MainWindow* window;

processArincMainFunction mainArincWraper;
void sleep(int64_t time);

int processArincMainFunction::createProcess(QString processName, void* _p_process) {
    qDebug() << "processArincMainFunction::createProcess " << processName;

    QtConcurrent::run([=]() {
        quint64 id = (quint64)QThread::currentThreadId();

        auto pinfo = std::make_shared<processInfo>(processName, id, _p_process);

        lock.lock();
        process.insert(id, pinfo);
        processOrder.append(id);
        lock.unlock();

        // Now, execute the function
        ((int(__cdecl *)())_p_process)();
    });

    return 0;
}

void processArincMainFunction::timeWait(int64_t _time) {
    quint64 id = (quint64)QThread::currentThreadId();
    lock.lock();
    auto it = process.find(id);
    if (it == process.end()) {
        lock.unlock();
        return;
    }
    auto pinfo = it.value();
    lock.unlock();

    qDebug() << "processArincMainFunction::timeWait" << _time << pinfo->name;

    std::unique_lock<std::mutex> lk(pinfo->mtx);
    pinfo->pause = _time;
    getTime(&(pinfo->timestop));
    pinfo->is_waiting = true;

    pinfo->cv.wait(lk, [pinfo]() {
        return !pinfo->is_waiting.load();
    });

    // After being notified, is_waiting is already set to false
}

void processArincMainFunction::periodicWait(void) {
    quint64 id = (quint64)QThread::currentThreadId();
    lock.lock();
    auto it = process.find(id);
    if (it == process.end()) {
        lock.unlock();
        return;
    }
    auto pinfo = it.value();
    lock.unlock();

    qDebug() << "processArincMainFunction::periodicWait" << pinfo->name;

    std::unique_lock<std::mutex> lk(pinfo->mtx);
    pinfo->is_waiting = true;

    pinfo->cv.wait(lk, [pinfo]() {
        return !pinfo->is_waiting.load();
    });

    // After being notified, is_waiting is already set to false
}

void processArincMainFunction::START_ALL_PROCESS(bool* autoMode, int sleepTime) {
    qDebug() << "processArincMainFunction::START_ALL_PROCESS";

    while (1) {
        for (int i = 0; i < processOrder.size(); i++) {
            quint64 id = processOrder[i];
            lock.lock();
            auto it = process.find(id);
            if (it == process.end()) {
                lock.unlock();
                continue;
            }
            auto pinfo = it.value();
            lock.unlock();

            qDebug() << "RUNPROCESS:" << pinfo->name;

            // Wait until the process is waiting
            {
                std::unique_lock<std::mutex> lk(pinfo->mtx);
                while (!pinfo->is_waiting.load()) {
                    lk.unlock();
                    sleep(1);
                    lk.lock();
                }

                int64_t tnow;
                getTime(&tnow);
                if (tnow - pinfo->timestop >= pinfo->pause) {
                    // Time to wake up the process
                    pinfo->is_waiting = false;
                    pinfo->cv.notify_one();
                }
            }

            // Wait until the process starts waiting again
            {
                std::unique_lock<std::mutex> lk(pinfo->mtx);
                while (pinfo->is_waiting.load()) {
                    lk.unlock();
                    sleep(1);
                    lk.lock();
                }
            }
        }

        fflush(stdout);

        QMetaObject::invokeMethod(window, []() {
            window->updateWindowsMemoryBlock();
            window->updateWindowsPorts();
        });

        if (!*autoMode)
            return;

        sleep(sleepTime);
    }
}
