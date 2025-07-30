-- IF
select if (1 > 2, 2, 3); -- 3
select if (1 < 2, 'yes', 'no'); -- yes
select if (strcmp('test', 'test1'), 'no', 'yes'); -- no
select if (NULL, 'yes', 'no'); -- 'no'
select if (NULL, NULL, NULL); -- NULL

-- ISNULL
select isnull(1);   -- 0
select isnull(0);   -- 0
select isnull(NULL);    -- 1
select isnull(1 + 1);   -- 0
select isnull(1 / 0);   -- 1

-- INTERVAL
select interval(23, 1, 15, 17, 30, 44, 200);    -- 3
select interval(10, 1, 10, 100, 1000);          -- 2
select interval(22, 23, 30, 44, 200);           -- 0
select interval(NULL, 23, 30, 44, 200);         -- -1
