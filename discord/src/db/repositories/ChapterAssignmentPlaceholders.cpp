// Associated Header Include
#include "db/repositories/ChapterAssignmentPlaceholders.hpp"

void ChapterAssignmentPlaceholdersRepository::create(pqxx::transaction_base &txn, int chapter_id, int task_id) {
  txn.exec(
      "INSERT INTO chapter_assignment_placeholders (chapter_id, task_id) VALUES ($1, $2)",
      pqxx::params(chapter_id, task_id));
}

void ChapterAssignmentPlaceholdersRepository::removeOne(pqxx::transaction_base &txn, int chapter_id, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignment_placeholders"
      " WHERE id = ("
      "   SELECT id FROM chapter_assignment_placeholders"
      "   WHERE chapter_id = $1 AND task_id = $2"
      "   ORDER BY id LIMIT 1"
      " )",
      pqxx::params(chapter_id, task_id));
}

void ChapterAssignmentPlaceholdersRepository::removeAll(pqxx::transaction_base &txn, int chapter_id, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignment_placeholders WHERE chapter_id = $1 AND task_id = $2",
      pqxx::params(chapter_id, task_id));
}

int ChapterAssignmentPlaceholdersRepository::count(pqxx::transaction_base &txn, int chapter_id, int task_id) {
  const auto result = txn.exec(
      "SELECT COUNT(*) FROM chapter_assignment_placeholders WHERE chapter_id = $1 AND task_id = $2",
      pqxx::params(chapter_id, task_id));
  return result[0][0].as<int>();
}

bool ChapterAssignmentPlaceholdersRepository::existsForChapter(pqxx::transaction_base &txn, int chapter_id) {
  const auto result = txn.exec(
      "SELECT 1 FROM chapter_assignment_placeholders WHERE chapter_id = $1 LIMIT 1",
      pqxx::params(chapter_id));
  return !result.empty();
}

void ChapterAssignmentPlaceholdersRepository::removeAllForTaskInSeries(pqxx::transaction_base &txn, int series_id, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignment_placeholders cap"
      " USING chapters c"
      " WHERE cap.chapter_id = c.id AND c.series_id = $1 AND cap.task_id = $2",
      pqxx::params(series_id, task_id));
}

void ChapterAssignmentPlaceholdersRepository::removeOnePerChapterForTaskInSeries(pqxx::transaction_base &txn, int series_id, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignment_placeholders"
      " WHERE id IN ("
      "   SELECT DISTINCT ON (cap.chapter_id) cap.id"
      "   FROM chapter_assignment_placeholders cap"
      "   JOIN chapters c ON c.id = cap.chapter_id"
      "   WHERE c.series_id = $1 AND cap.task_id = $2"
      "   ORDER BY cap.chapter_id, cap.id ASC"
      " )",
      pqxx::params(series_id, task_id));
}

void ChapterAssignmentPlaceholdersRepository::removeAllByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignment_placeholders WHERE task_id = $1",
      pqxx::params(task_id));
}
