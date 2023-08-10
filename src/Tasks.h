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
// }
// API END

}; // namespace Tasks

#endif
