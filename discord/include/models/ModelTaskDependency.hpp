#pragma once

struct TaskDependency {
  int task_id;
  int depends_on_task_id;
};
