-- gh-2991 - Tarantool asserts on box.cfg.replication update if one of
-- servers is dead
box.schema.user.grant('guest', 'replication')

box.cfg{replication_timeout=0.05, replication={}}

box.cfg{replication = {'127.0.0.1:12345', box.cfg.listen}}

box.schema.user.revoke('guest', 'replication')
