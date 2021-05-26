-- 

CREATE TABLE `Agile_usage` (
  `period_from` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `period_to` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `power_usage` double DEFAULT NULL
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Indexes for table `Agile_usage`
--
ALTER TABLE `Agile_usage`
  ADD PRIMARY KEY (`period_from`);

