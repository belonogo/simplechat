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