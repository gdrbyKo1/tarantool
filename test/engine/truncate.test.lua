test_run = require('test_run').new()
engine = test_run:get_cfg('engine')

fiber =  require('fiber')

--
-- Check that space truncation is forbidden in a transaction.
--
s = box.schema.create_space('test', {engine = engine})
_ = s:create_index('pk')
_ = s:insert{123}
box.begin()
s:truncate()
box.commit()
s:select()
s:drop()

--
-- Truncate space with no indexes.
--
s = box.schema.create_space('test', {engine = engine})
s:truncate()
s:drop()

--
-- Truncate empty space.
--
s = box.schema.create_space('test', {engine = engine})
_ = s:create_index('pk')
s:truncate()
s:select()
s:drop()

--
-- Truncate non-empty space.
--
s = box.schema.create_space('test', {engine = engine})
_ = s:create_index('i1', {parts = {1, 'unsigned'}})
_ = s:create_index('i2', {parts = {2, 'unsigned'}})
_ = s:create_index('i3', {parts = {3, 'string'}})
_ = s:insert{1, 3, 'a'}
_ = s:insert{2, 2, 'b'}
_ = s:insert{3, 1, 'c'}
s:truncate()
s.index.i1:select()
s.index.i2:select()
s.index.i3:select()
_ = s:insert{10, 30, 'x'}
_ = s:insert{20, 20, 'y'}
_ = s:insert{30, 10, 'z'}
s.index.i1:select()
s.index.i2:select()
s.index.i3:select()
s:drop()

--
-- Check that space truncation is linearizable.
--
-- Create a space with several indexes and start three fibers:
-- 1st and 3rd update the space, 2nd truncates it. Then wait
-- until all fibers are done. The space should contain data
-- inserted by the 3rd fiber.
--
-- Note, this is guaranteed to be true only if space updates
-- don't yield, which is always true for memtx and is true
-- for vinyl in case there's no data on disk, as in this case.
--
s = box.schema.create_space('test', {engine = engine})
_ = s:create_index('i1', {parts = {1, 'unsigned'}})
_ = s:create_index('i2', {parts = {2, 'unsigned'}})
_ = s:create_index('i3', {parts = {3, 'string'}})
_ = s:insert{1, 1, 'a'}
_ = s:insert{2, 2, 'b'}
_ = s:insert{3, 3, 'c'}
c = fiber.channel(3)
test_run:cmd("setopt delimiter ';'")
fiber.create(function()
    box.begin()
    s:replace{1, 10, 'aa'}
    s:replace{2, 20, 'bb'}
    s:replace{3, 30, 'cc'}
    box.commit()
    c:put(true)
end)
fiber.create(function()
    s:truncate()
    c:put(true)
end)
fiber.create(function()
    box.begin()
    s:replace{1, 100, 'aaa'}
    s:replace{2, 200, 'bbb'}
    s:replace{3, 300, 'ccc'}
    box.commit()
    c:put(true)
end)
test_run:cmd("setopt delimiter ''");
for i = 1, 3 do c:get() end
s.index.i1:select()
s.index.i2:select()
s.index.i3:select()
s:drop()

--
-- Check that space truncation is persistent.
--
-- The test checks the following cases:
-- - Create and truncate before snapshot
-- - Create before snapshot, truncate after snapshot
-- - Create and truncate after snapshot
--
s1 = box.schema.create_space('test1', {engine = engine})
_ = s1:create_index('i1', {parts = {1, 'unsigned'}})
_ = s1:create_index('i2', {parts = {2, 'unsigned'}})
_ = s1:insert{1, 3}
_ = s1:insert{2, 2}
_ = s1:insert{3, 1}
s1:truncate()
_ = s1:insert{123, 321}
s2 = box.schema.create_space('test2', {engine = engine})
_ = s2:create_index('i1', {parts = {1, 'unsigned'}})
_ = s2:create_index('i2', {parts = {2, 'unsigned'}})
_ = s2:insert{10, 30}
_ = s2:insert{20, 20}
_ = s2:insert{30, 10}
box.snapshot()
_ = s1:insert{321, 123}
s2:truncate()
_ = s2:insert{456, 654}
s3 = box.schema.create_space('test3', {engine = engine})
_ = s3:create_index('i1', {parts = {1, 'unsigned'}})
_ = s3:create_index('i2', {parts = {2, 'unsigned'}})
_ = s3:insert{100, 300}
_ = s3:insert{200, 200}
_ = s3:insert{300, 100}
s3:truncate()
_ = s3:insert{789, 987}
test_run:cmd('restart server default')
s1 = box.space.test1
s2 = box.space.test2
s3 = box.space.test3
s1.index.i1:select()
s1.index.i2:select()
s2.index.i1:select()
s2.index.i2:select()
s3.index.i1:select()
s3.index.i2:select()
s1:drop()
s2:drop()
s3:drop()
