-- 

CREATE TABLE `Agile_tariffs` (
  `period_start` datetime NOT NULL,
  `period_end` datetime NOT NULL,
  `price` float NOT NULL
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Indexes for table `Agile_tariffs`
--
ALTER TABLE `Agile_tariffs`
  ADD UNIQUE KEY `start_idx` (`period_start`);

