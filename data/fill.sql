BEGIN TRANSACTION;


INSERT INTO tasks (title, description, status, due_date)
VALUES
  ('Buy groceries', 'Milk, eggs, bread', 0, strftime('%s','2025-07-19')),
  ('Finish report',   'Send to manager',    0, strftime('%s','2025-07-18')),
  ('Call Alice',      '',                    1, NULL);

INSERT OR IGNORE INTO tags (name)
VALUES ('home'), ('work'), ('urgent'), ('phone');

-- link tasks to tags
INSERT INTO task_tags (task_id, tag_id)
VALUES
  (1,1),
  (1,3),
  (2,2),   
  (2,3),
  (3,4);

COMMIT;
