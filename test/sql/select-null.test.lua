test_run = require('test_run').new()
engine = test_run:get_cfg('engine')
box.sql.execute('pragma sql_default_engine=\''..engine..'\'')

-- box.cfg()

-- create space
box.sql.execute("CREATE TABLE t3(id INT, a text, b TEXT, PRIMARY KEY(id))")

-- Debug
-- box.sql.execute("PRAGMA vdbe_debug=ON ; INSERT INTO zoobar VALUES (111, 222, 'c3', 444)")

-- Seed entries
box.sql.execute("INSERT INTO t3 VALUES(1, 'abc',NULL)");
box.sql.execute("INSERT INTO t3 VALUES(2, NULL,'xyz')");

-- Select must return properly decoded `NULL`
box.sql.execute("SELECT * FROM t3")

-- Cleanup
box.sql.execute("DROP TABLE t3")

-- Debug
-- require("console").start()
