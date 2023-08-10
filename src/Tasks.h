#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <variant>

namespace Tasks {

// DEFINITIONS
class TaskRef;
void NoOp(const TaskRef *ref);
bool True(const TaskRef *ref);
// DEFINITIONS END

// INTERNALS
using void_type =
    std::variant<std::function<void(const TaskRef *)>, std::function<void()>>;
using bool_type =
    std::variant<std::function<bool(const TaskRef *)>, std::function<bool()>>;

struct fn_discriminator {
  std::function<void(const TaskRef *)>
  operator()(const std::function<void(const TaskRef *)> &fn) const {
    return [fn](const TaskRef *ref) { fn(ref); };
  }
  std::function<void(const TaskRef *)>
  operator()(const std::function<void()> &fn) const {
    return [fn](const TaskRef *ignored) { fn(); };
  }

  std::function<bool(const TaskRef *)>
  operator()(const std::function<bool(const TaskRef *)> &fn) const {
    // BUT WHY DO I SUCK AT PROGRAMMING
    return [fn](const TaskRef *ref) { return fn(ref); };
  }
  std::function<bool(const TaskRef *)>
  operator()(const std::function<bool()> &fn) const {
    return [fn](const TaskRef *ignored) { return fn(); };
  }
};
const fn_discriminator discriminator;
// INTERNALS END

// CLASSES
class TaskRef {};

class ITask : public TaskRef {
protected:
  boolean started = false;

public:
  virtual ~ITask(){};

  virtual boolean isStarted() = 0;
  virtual void setup() = 0;
  virtual boolean isFinished() = 0;
  virtual void finish() = 0;
};

class Task : public ITask {
private:
  std::function<void(const TaskRef *)> setup_fn;
  std::function<bool(const TaskRef *)> check_fn;
  std::function<void(const TaskRef *)> callback_fn;

public:
  Task(void_type setupFn = NoOp, bool_type checkFn = True,
       void_type callbackFn = NoOp)
      : setup_fn(std::visit(discriminator, setupFn)),
        check_fn(std::visit(discriminator, checkFn)),
        callback_fn(std::visit(discriminator, callbackFn)) {}

  ~Task() { Serial.println("destructor called"); }

  virtual boolean isStarted() override { return started; }
  virtual void setup() override {
    Serial.println("task init");
    setup_fn(this);
    started = true;
  }
  virtual boolean isFinished() override { return check_fn(this); }
  virtual void finish() override {
    Serial.println("task finish");
    callback_fn(this);
  }
};
// CLASSES END

// MAIN LOGIC
std::map<const TaskRef *, unsigned long> millisDataStore;
std::list<std::shared_ptr<Task>> tasks;

std::set<const TaskRef *> intervals;

void loop() {
  for (auto it = tasks.begin(); it != tasks.end();) {
    auto &task = **it;

    if (!task.isStarted()) {
      task.setup();
    }
    if (task.isFinished()) {
      task.finish();
      it = tasks.erase(it);
    } else {
      ++it;
    }
  }
}
// MAIN LOGIC END

// API
void NoOp(const TaskRef *ref) {}

bool True(const TaskRef *ref) { return true; }

TaskRef *queueTask(Task *task) {
  tasks.push_back(std::move(std::shared_ptr<Task>(task)));
  return tasks.back().get();
}

void setMillis(const TaskRef *ref, unsigned long millis) {
  millisDataStore[ref] = millis;
}

unsigned long getMillis(const TaskRef *ref) {
  auto it = millisDataStore.find(ref);
  return it != millisDataStore.end() ? it->second : 0;
}

void clearMillis(const TaskRef *ref) { millisDataStore.erase(ref); }

void setTimeout(std::function<void()> callback, unsigned long ms) {
  tasks.push_back(std::make_shared<Task>(
      [](const TaskRef *ref) { setMillis(ref, millis()); },
      [ms](const TaskRef *ref) { return millis() - getMillis(ref) >= ms; },
      [callback](const TaskRef *ref) {
        clearMillis(ref);
        callback();
      }));
}

TaskRef *setInterval(std::function<void()> doWork, unsigned long ms,
                     bool immediate = true) {
  tasks.push_back(std::make_shared<Task>(
      [ms, immediate](const TaskRef *ref) {
        intervals.emplace(ref);
        setMillis(ref, millis() - ms * immediate);
      },
      [ms, doWork](const TaskRef *ref) {
        if (intervals.find(ref) == intervals.end()) {
          return true;
        }

        auto curr_millis = millis();
        if (curr_millis - getMillis(ref) >= ms) {
          setMillis(ref, curr_millis);
          doWork();
        }

        return false;
      },
      NoOp));
  return tasks.back().get();
}

void clearInterval(const TaskRef *ref) { intervals.erase(ref); }

// #include <type_traits>
// template <class T, typename std::enable_if<
//                        std::is_base_of<ITask, T>::value>::type * = nullptr>
// void queueTask(const T &task) {
//   static_assert(std::is_base_of_v<ITask, T>, "Invalid task type");
//   tasks.push_back(std::make_unique<T>(std::move(task)));
// }
// API END

}; // namespace Tasks

// class ITask {
// public:
//   virtual void setData(uint8_t *data, size_t size) = 0;
//   virtual uint8_t *getData() = 0;
//   virtual size_t getSize() = 0;

//   virtual unsigned long getPrevMillis() = 0;
//   virtual void setPrevMillis(unsigned long millis) = 0;
// }; // namespace Tasksclass ITask

// // template <typename T>
// // using TaskData = std::variant<std::unique_ptr<T>, std::shared_ptr<T>>;
// // std::map<std::pair<ITask, const char *>, TaskData<std::any>> store;

// typedef std::function<void(ITask *)> SetupFn;
// typedef std::function<boolean(ITask *)> CheckFn;
// typedef std::function<void(ITask *)> CallbackFn;

// void NoOp(ITask *) {}
// boolean AlwaysTrue(ITask *) { return true; }

// // void setMillis(ITask *task, unsigned long millis) {
// //   if (task->getData() == NULL) {
// //     size_t size = sizeof(unsigned long);
// //     auto data = (unsigned long *)malloc(size);
// //     *data = millis;
// //     task->setData((uint8_t *)data, size);
// //   } else {
// //     *((unsigned long *)task->getData()) = millis;
// //   }
// // }

// // unsigned long getMillis(ITask *task) {
// //   return task->getData() == NULL ? 0 : *((unsigned long
// *)task->getData());
// // }

// void setMillis(ITask *task, unsigned long millis) {
//   task->setPrevMillis(millis);
// }

// unsigned long getMillis(ITask *task) { return task->getPrevMillis(); }

// class Task : public ITask {
// private:
//   boolean started = false;
//   uint8_t *data = NULL;
//   size_t size = -1;
//   unsigned long prev_millis;

//   // std::map<const char *, TaskData<std::any>> data;

//   SetupFn setupFn;
//   // std::function<void()> workFn;
//   CheckFn checkFn;
//   CallbackFn callbackFn;

// public:
//   Task(SetupFn setupFn, CheckFn checkFn, CallbackFn callbackFn)
//       : setupFn(setupFn), checkFn(checkFn), callbackFn(callbackFn) {}

//   ~Task() {
//     // Serial.println("destructor called");
//   }

//   virtual void setData(uint8_t *data, size_t size) override;
//   virtual uint8_t *getData() override;
//   virtual size_t getSize() override;

//   virtual unsigned long getPrevMillis() override;
//   virtual void setPrevMillis(unsigned long millis) override;

//   virtual void setup();
//   virtual boolean isFinished();
//   virtual void finish();
//   boolean isStarted();
// };

// uint8_t *Task::getData() { return this->data; }
// size_t Task::getSize() { return this->size; }

// void Task::setData(uint8_t *data, size_t size) {
//   this->data = data;
//   this->size = size;
// }

// unsigned long Task::getPrevMillis() { return prev_millis; }
// void Task::setPrevMillis(unsigned long millis) { prev_millis = millis; }

// void Task::setup() {
//   this->setupFn(this);
//   started = true;
// }
// boolean Task::isFinished() { return this->checkFn(this); }
// void Task::finish() { this->callbackFn(this); }
// boolean Task::isStarted() { return this->started; }

// class DependentTask : public Task {
// private:
//   std::vector<std::shared_ptr<boolean>> dependencies;
//   boolean aborted = false;

// public:
//   DependentTask(std::vector<std::shared_ptr<boolean>> dependencies,
//                 SetupFn setupFn, CheckFn checkFn, CallbackFn callbackFn)
//       : Task(setupFn, checkFn, callbackFn), dependencies(dependencies) {}

//   boolean isFinished() override;
//   void finish() override;
// };

// boolean DependentTask::isFinished() {
//   // if a dependency is not alive anymore we kill the task
//   for (auto &dep : dependencies) {
//     if (!(*dep)) {
//       return aborted = true;
//     }
//   }
//   return Task::isFinished();
// }

// void DependentTask::finish() {
//   if (aborted) {
//     return;
//   }
//   Task::finish();
// }

#endif
