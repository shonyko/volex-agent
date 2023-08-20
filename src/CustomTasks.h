#ifndef CUSTOM_TASKS_H
#define CUSTOM_TASKS_H

#include <Arduino.h>
#include <Tasks.h>
#include <memory>

class AdapterTask : public Tasks::Task {
public:
  AdapterTask()
      : Task([&](const Tasks::TaskRef *ref) { init(ref); },
             [&](const Tasks::TaskRef *ref) { return check(ref); },
             [&](const Tasks::TaskRef *ref) { dispatch(ref); }) {
    Serial.println("adapter constructor");
  }

  ~AdapterTask() { Serial.println("adapter destructor"); }

  virtual void init(const Tasks::TaskRef *ref) { init(); }
  virtual bool check(const Tasks::TaskRef *ref) { return check(); }
  virtual void dispatch(const Tasks::TaskRef *ref) { dispatch(); }

  virtual void init() { Serial.println("adapter init"); }
  virtual bool check() {
    Serial.println("adapter check");
    return true;
  }
  virtual void dispatch() { Serial.println("adapter dispatch"); }
};

class TimedTask : public AdapterTask {
private:
  unsigned long prev_millis;
  unsigned long delay;
  bool immediate;

  std::function<bool()> work;
  std::function<void()> callback;

public:
  TimedTask(std::function<bool()> work, std::function<void()> callback,
            unsigned long delay, bool immediate = true)
      : work(work), callback(callback), delay(delay), immediate(immediate) {}

  ~TimedTask() { Serial.println("timed destructor"); }

  virtual void init() {
    prev_millis = millis() - delay * immediate;
    Serial.println("timed setup: " + String(prev_millis));
  }
  virtual bool check() {
    auto curr_millis = millis();
    if (curr_millis - prev_millis >= delay) {
      prev_millis = curr_millis;
      return work();
    }
    return false;
  }
  virtual void dispatch() {
    callback();
    Serial.println("timed dispatch");
  }
};

class CompositeTask : public AdapterTask {
protected:
  std::unique_ptr<Tasks::Task> task = nullptr;

public:
  CompositeTask(Tasks::Task *t)
      : task(std::move(std::unique_ptr<Tasks::Task>(t))) {}

  ~CompositeTask() { Serial.println("composite destructor"); }

  virtual void init() { task->setup(); }
  virtual bool check() { return task->isFinished(); }
  virtual void dispatch() { task->finish(); }
};

class DependentTask : public CompositeTask {
private:
  std::vector<std::shared_ptr<boolean>> deps;

  bool aborted = false;

public:
  DependentTask(std::vector<std::shared_ptr<boolean>> dependencies,
                Tasks::Task *task)
      : CompositeTask(task), deps(std::move(dependencies)) {}

  // Idk why this does not work
  DependentTask(std::vector<std::shared_ptr<boolean>> dependencies,
                Tasks::void_type setupFn, Tasks::bool_type checkFn,
                Tasks::void_type callbackFn)
      : DependentTask(deps, new Tasks::Task(setupFn, checkFn, callbackFn)) {}

  ~DependentTask() { Serial.println("dependent destructor called"); }

  virtual bool check() override {
    for (auto &dep : deps) {
      if (!*dep) {
        return aborted = true;
      }
    }
    return task->isFinished();
  }

  virtual void dispatch() override {
    if (!aborted) {
      task->finish();
    }
  }
};

#endif