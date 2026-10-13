--
-- regression test for compatible Oracle pivot and unpivot
--
drop table if exists olympic_medal_winners;
create table olympic_medal_winners (   
  olympic_year int,  
  sport        varchar2( 30 ),  
  gender       varchar2( 1 ),  
  event        varchar2( 128 ),  
  medal        varchar2( 10 ),  
  noc          varchar2( 3 ),  
  athlete      varchar2( 128 ) 
);

Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Individual','Gold','KOR','KU Bonchan');
Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Individual','Silver','FRA','VALLADONT Jean-Charles');
Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Individual','Bronze','USA','ELLISON Brady');
Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Team','Gold','KOR','Republic of Korea');
Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Team','Bronze','AUS','Australia');
Insert into olympic_medal_winners  values (2016,'Archery','M','Men''s Team','Silver','USA','United States');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Floor Exercise','Gold','GBR','WHITLOCK Max');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Floor Exercise','Bronze','BRA','MARIANO Arthur');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Floor Exercise','Silver','BRA','HYPOLITO Diego');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Horizontal Bar','Gold','GER','HAMBUECHEN Fabian');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Horizontal Bar','Bronze','GBR','WILSON Nile');
Insert into olympic_medal_winners  values (2016,'Artistic Gymnastics','M','Men''s Horizontal Bar','Silver','USA','LEYVA Danell');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 10,000m','Gold','GBR','FARAH Mohamed');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 10,000m','Bronze','ETH','TOLA Tamirat');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 10,000m','Silver','KEN','TANUI Paul Kipngetich');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 100m','Gold','JAM','BOLT Usain');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 100m','Silver','USA','GATLIN Justin');
Insert into olympic_medal_winners  values (2016,'Athletics','M','Men''s 100m','Bronze','CAN','DE GRASSE Andre');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Gold','CHN','Zhang');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Bronze','GBR','Langridge');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Bronze','GBR','Ellis');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Silver','MAS','Tan');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Silver','MAS','Goh');
Insert into olympic_medal_winners  values (2016,'Badminton','M','Men''s Doubles','Gold','CHN','Fu');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Gold','BRA','Cerutti');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Gold','BRA','Oscar Schmidt');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Silver','ITA','Nicolai');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Silver','ITA','Lupo');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Bronze','NED','Meeuwsen');
Insert into olympic_medal_winners  values (2016,'Beach Volleyball','M','Men','Bronze','NED','Brouwer');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Bantam (56kg)','Gold','CUB','RAMIREZ Robeisy');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Bantam (56kg)','Bronze','UZB','AKHMADALIEV Murodjon');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Bantam (56kg)','Bronze','RUS','NIKITIN Vladimir');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Bantam (56kg)','Silver','USA','STEVENSON Shakur');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Fly (52kg)','Gold','UZB','ZOIROV Shakhobidin');
Insert into olympic_medal_winners  values (2016,'Boxing','M','Men''s Fly (52kg)','Bronze','CHN','HU Jianguan');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Gold','SVK','PETER Skantar');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Bronze','FRA','GAUTHIER Klauss');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Bronze','FRA','MATTHIEU Peche');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Silver','GBR','RICHARD Hounslow');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Silver','GBR','DAVID Florence');
Insert into olympic_medal_winners  values (2016,'Canoe Slalom','M','Canoe Double (C2) Men','Gold','SVK','LADISLAV Skantar');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Gold','GER','Brendel');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Bronze','UKR','Mishchuk');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Bronze','UKR','Ianchuk');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Silver','BRA','Queiroz dos Santos');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Silver','BRA','de Souza Silva');
Insert into olympic_medal_winners  values (2016,'Canoe Sprint','M','Men''s Canoe Double 1000m','Gold','GER','Vandrey');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Individual Time Trial','Gold','SUI','CANCELLARA Fabian');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Individual Time Trial','Bronze','GBR','FROOME Christopher');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Individual Time Trial','Silver','NED','DUMOULIN Tom');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Road Race','Gold','BEL','VAN AVERMAET Greg');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Road Race','Silver','DEN','FUGLSANG Jakob');
Insert into olympic_medal_winners  values (2016,'Cycling Road','M','Men''s Road Race','Bronze','POL','MAJKA Rafal');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Keirin','Gold','GBR','KENNY Jason');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Keirin','Bronze','MAS','AWANG Azizulhasni');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Keirin','Silver','NED','BUCHLI Matthijs');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Omnium','Gold','ITA','VIVIANI Elia');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Omnium','Bronze','DEN','HANSEN Lasse Norman');
Insert into olympic_medal_winners  values (2016,'Cycling Track','M','Men''s Omnium','Silver','GBR','CAVENDISH Mark');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 10m Platform','Gold','CHN','CHEN Aisen');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 10m Platform','Bronze','USA','BOUDIA David');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 10m Platform','Silver','MEX','SANCHEZ German');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 3m Springboard','Gold','CHN','CAO Yuan');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 3m Springboard','Silver','GBR','LAUGHER Jack');
Insert into olympic_medal_winners  values (2016,'Diving','M','Men''s 3m Springboard','Bronze','GER','HAUSDING Patrick');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Individual','Gold','GBR','DUJARDIN Charlotte');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Individual','Bronze','GER','BRORING-SPREHE Kristina');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Individual','Silver','GER','WERTH Isabell');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Team','Gold','GER','Germany');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Team','Bronze','USA','United States');
Insert into olympic_medal_winners  values (2016,'Equestrian','X','Dressage Team','Silver','GBR','Great Britain');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Individual','Gold','ITA','GAROZZO Daniele');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Individual','Silver','USA','MASSIALAS Alexander');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Individual','Bronze','RUS','SAFIN Timur');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Team','Gold','RUS','Russian Federation');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Team','Bronze','USA','United States');
Insert into olympic_medal_winners  values (2016,'Fencing','M','Men''s Foil Team','Silver','FRA','France');
Insert into olympic_medal_winners  values (2016,'Handball','M','Men','Gold','DEN','Denmark');
Insert into olympic_medal_winners  values (2016,'Handball','M','Men','Silver','FRA','France');
Insert into olympic_medal_winners  values (2016,'Handball','M','Men','Bronze','GER','Germany');
Insert into olympic_medal_winners  values (2016,'Handball','W','Women','Gold','RUS','Russian Federation');
Insert into olympic_medal_winners  values (2016,'Handball','W','Women','Silver','FRA','France');
Insert into olympic_medal_winners  values (2016,'Handball','W','Women','Bronze','NOR','Norway');
Insert into olympic_medal_winners  values (2016,'Hockey','M','Men','Gold','ARG','Argentina');
Insert into olympic_medal_winners  values (2016,'Hockey','M','Men','Silver','BEL','Belgium');
Insert into olympic_medal_winners  values (2016,'Hockey','M','Men','Bronze','GER','Germany');
Insert into olympic_medal_winners  values (2016,'Hockey','W','Women','Gold','GBR','Great Britain');
Insert into olympic_medal_winners  values (2016,'Hockey','W','Women','Silver','NED','Netherlands');
Insert into olympic_medal_winners  values (2016,'Hockey','W','Women','Bronze','GER','Germany');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men +100 kg','Gold','FRA','RINER Teddy');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men +100 kg','Bronze','BRA','SILVA Rafael');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men +100 kg','Bronze','ISR','SASSON Or');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men +100 kg','Silver','JPN','HARASAWA Hisayoshi');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men -100 kg','Gold','CZE','KRPALEK Lukas');
Insert into olympic_medal_winners  values (2016,'Judo','M','Men -100 kg','Bronze','FRA','MARET Cyrille');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','M','Men''s Individual','Gold','RUS','LESUN Alexander');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','M','Men''s Individual','Silver','UKR','TYMOSHCHENKO Pavlo');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','M','Men''s Individual','Bronze','MEX','HERNANDEZ USCANGA Ismael Marcelo');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','W','Women''s Individual','Gold','AUS','ESPOSITO Chloe');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','W','Women''s Individual','Silver','FRA','CLOUVEL Elodie');
Insert into olympic_medal_winners  values (2016,'Modern Pentathlon','W','Women''s Individual','Bronze','POL','NOWACKA Oktawia');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Group All-Around','Gold','RUS','Russian Federation');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Group All-Around','Bronze','BUL','Bulgaria');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Group All-Around','Silver','ESP','Spain');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Individual All-Around','Gold','RUS','MAMUN Margarita');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Individual All-Around','Silver','RUS','KUDRYAVTSEVA Yana');
Insert into olympic_medal_winners  values (2016,'Rhythmic Gymnastics','W','Individual All-Around','Bronze','UKR','RIZATDINOVA Ganna');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Gold','FRA','Azou');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Bronze','NOR','Brun');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Bronze','NOR','Strandli');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Silver','IRL','O''Donovan');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Silver','IRL','O''Donovan');
Insert into olympic_medal_winners  values (2016,'Rowing','M','Lightweight Men''s Double Sculls','Gold','FRA','Houin');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Gold','CRO','Fantela');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Bronze','GRE','Kagialis');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Bronze','GRE','Mantis');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Silver','AUS','Ryan');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Silver','AUS','Belcher');
Insert into olympic_medal_winners  values (2016,'Sailing','M','470 Men','Gold','CRO','Marenic');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Backstroke','Gold','USA','MURPHY Ryan');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Backstroke','Bronze','USA','PLUMMER David');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Backstroke','Silver','CHN','XU Jiayu');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Breaststroke','Gold','GBR','PEATY Adam');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Breaststroke','Bronze','USA','MILLER Cody');
Insert into olympic_medal_winners  values (2016,'Swimming','M','Men''s 100m Breaststroke','Silver','RSA','VAN DER BURGH Cameron');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Singles','Gold','CHN','MA Long');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Singles','Bronze','JPN','MIZUTANI Jun');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Singles','Silver','CHN','ZHANG Jike');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Team','Gold','CHN','China');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Team','Bronze','GER','Germany');
Insert into olympic_medal_winners  values (2016,'Table Tennis','M','Men''s Team','Silver','JPN','Japan');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men +80kg','Gold','AZE','ISAEV Radik');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men +80kg','Bronze','KOR','CHA Dongmin');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men +80kg','Bronze','BRA','SIQUEIRA Maicon');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men +80kg','Silver','NIG','ISSOUFOU ALFAGA Abdoulrazak');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men -58kg','Gold','CHN','ZHAO Shuai');
Insert into olympic_medal_winners  values (2016,'Taekwondo','M','Men -58kg','Silver','THA','HANPRAB Tawin');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Gold','ESP','Lopez');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Bronze','USA','Johnson');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Bronze','USA','Sock');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Silver','ROU','Tecau');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Silver','ROU','Mergea');
Insert into olympic_medal_winners  values (2016,'Tennis','M','Men''s Doubles','Gold','ESP','Nadal');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','M','Men','Gold','BLR','HANCHAROU Uladzislau');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','M','Men','Silver','CHN','DONG Dong');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','M','Men','Bronze','CHN','GAO Lei');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','W','Women','Gold','CAN','MACLENNAN Rosannagh');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','W','Women','Silver','GBR','PAGE Bryony');
Insert into olympic_medal_winners  values (2016,'Trampoline Gymnastics','W','Women','Bronze','CHN','LI Dan');
Insert into olympic_medal_winners  values (2016,'Triathlon','M','Men','Gold','GBR','BROWNLEE Alistair');
Insert into olympic_medal_winners  values (2016,'Triathlon','M','Men','Silver','GBR','BROWNLEE Jonathan');
Insert into olympic_medal_winners  values (2016,'Triathlon','M','Men','Bronze','RSA','SCHOEMAN Henri');
Insert into olympic_medal_winners  values (2016,'Triathlon','W','Women','Gold','USA','JORGENSEN Gwen');
Insert into olympic_medal_winners  values (2016,'Triathlon','W','Women','Silver','SUI','SPIRIG HUG Nicola');
Insert into olympic_medal_winners  values (2016,'Triathlon','W','Women','Bronze','GBR','HOLLAND Vicky');
Insert into olympic_medal_winners  values (2016,'Volleyball','M','Men','Gold','BRA','Brazil');
Insert into olympic_medal_winners  values (2016,'Volleyball','M','Men','Silver','ITA','Italy');
Insert into olympic_medal_winners  values (2016,'Volleyball','M','Men','Bronze','USA','United States');
Insert into olympic_medal_winners  values (2016,'Volleyball','W','Women','Gold','CHN','China');
Insert into olympic_medal_winners  values (2016,'Volleyball','W','Women','Silver','SRB','Serbia');
Insert into olympic_medal_winners  values (2016,'Volleyball','W','Women','Bronze','USA','United States');
Insert into olympic_medal_winners  values (2016,'Water Polo','M','Men','Gold','SRB','Serbia');
Insert into olympic_medal_winners  values (2016,'Water Polo','M','Men','Silver','CRO','Croatia');
Insert into olympic_medal_winners  values (2016,'Water Polo','M','Men','Bronze','ITA','Italy');
Insert into olympic_medal_winners  values (2016,'Water Polo','W','Women','Gold','USA','United States');
Insert into olympic_medal_winners  values (2016,'Water Polo','W','Women','Silver','ITA','Italy');
Insert into olympic_medal_winners  values (2016,'Water Polo','W','Women','Bronze','RUS','Russian Federation');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s +105kg','Gold','GEO','TALAKHADZE Lasha');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s +105kg','Bronze','GEO','TURMANIDZE Irakli');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s +105kg','Silver','ARM','MINASYAN Gor');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s 105kg','Gold','UZB','NURUDINOV Ruslan');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s 105kg','Bronze','KAZ','ZAICHIKOV Alexandr');
Insert into olympic_medal_winners  values (2016,'Weightlifting','M','Men''s 105kg','Silver','ARM','MARTIROSYAN Simon');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 125 kg','Gold','TUR','AKGUL Taha');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 125 kg','Bronze','BLR','SAIDAU Ibrahim');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 125 kg','Bronze','GEO','PETRIASHVILI Geno');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 125 kg','Silver','IRI','GHASEMI Komeil Nemat');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 57 kg','Gold','GEO','KHINCHEGASHVILI Vladimer');
Insert into olympic_medal_winners  values (2016,'Wrestling','M','Men''s Freestyle 57 kg','Bronze','AZE','ALIYEV Haji');

select count(*) from olympic_medal_winners; -- 180

--1,
select * from olympic_medal_winners
pivot ( count(*) for medal in ( 
 'Gold' gold, 'Silver' silver, 'Bronze' bronze
))
order by noc
fetch first 6 rows only;

--2,
select * from (
 select noc, medal from olympic_medal_winners
)
pivot ( count(*) for medal in (
 'Gold' gold, 'Silver' silver, 'Bronze' bronze
))
order  by 2 desc, 3 desc, 4 desc
fetch first 5 rows only;

--3,
select * from (
 select noc, medal, sport, event, gender
 from olympic_medal_winners
)
pivot ( count(distinct sport ||'#'|| event ||'#'||gender ) for medal in (
 'Gold' gold, 'Silver' silver, 'Bronze' bronze
))
order  by 2 desc, 3 desc, 4 desc
fetch first 5 rows only;

--4,
select * from (
  select noc, medal, sport, event, gender, athlete
  from   olympic_medal_winners
)
pivot  (
  count( distinct sport ||'#'|| event ||'#'|| gender ) medals,
  count( distinct sport ) sports,
  listagg( athlete, ',') within group (order by athlete) athletes
  for medal in ( 'Gold' gold )
)
where  gold_medals > 1
order  by gold_medals, gold_sports, noc
fetch  first 5 rows only;

--5,
select * from (
  select noc, medal, sport, event, gender, athlete
  from   olympic_medal_winners
)
pivot  (
  count( distinct sport ||'#'|| event ||'#'|| gender ) medals,
  count( distinct sport ) sports,
  listagg( athlete, ',') within group (order by athlete) athletes
  for medal in ( 'Gold' gold )
)
where  noc like 'D%'
order  by gold_medals;

--6,
select * from ( 
  select noc, sport
  from   olympic_medal_winners
)
pivot (min('X') for sport in (
  'Archery' as arc, 'Athletics' as ath, 'Hockey' as hoc,
  'Judo' as jud, 'Sailing' as sai, 'Wrestling' as wre
  )
)
order  by noc
fetch  first 7 rows only;

--7,
drop table if exists olympic_medal_tables;
create table olympic_medal_tables as
select * from (
 select noc, medal, sport, event, gender
 from olympic_medal_winners
)
pivot ( count(distinct sport ||'#'|| event ||'#'||gender ) for medal in (
 'Gold' gold_medals, 'Silver' silver_medals, 'Bronze' bronze_medals
))
order  by 2 desc, 3 desc, 4 desc;

--8,
select * from olympic_medal_tables
unpivot (medal_count for medal_colour in (
  gold_medals as 'GOLD',
  silver_medals as 'SILVER',
  bronze_medals as 'BRONZE'
))
order  by noc
fetch  first 6 rows only;

drop table olympic_medal_tables purge;

--9,
create table olympic_medal_tables as
select * from (
 select noc, medal, sport, event, gender
 from olympic_medal_winners
)
pivot ( count(distinct sport ||'#'|| event ||'#'||gender ) medals,
  count(distinct sport) sports
  for medal in (
 'Gold' gold, 'Silver' silver, 'Bronze' bronze
))
order  by 2 desc, 4 desc, 6 desc;

select count(*) from olympic_medal_tables;

--10,
select * from olympic_medal_tables
unpivot ((medal_count, sport_count) for medal_colour in (
  (gold_medals, gold_sports) as 'GOLD',
  (silver_medals, silver_sports) as 'SILVER',
  (bronze_medals, bronze_sports) as 'BRONZE'
))
fetch first 9 rows only;

drop table olympic_medal_tables purge;

--11,
create table olympic_medal_tables as
select * from (
 select noc, medal, sport, event, gender, athlete
 from olympic_medal_winners
)
pivot ( count(distinct sport ||'#'|| event ||'#'||gender ) medals,
  listagg(athlete, ',') within group (order by athlete) athletes
  for medal in (
 'Gold' gold, 'Silver' silver, 'Bronze' bronze
))
order  by 2 desc, 4 desc, 6 desc;

select count(*) from olympic_medal_tables;

--12,
select * from olympic_medal_tables
unpivot ((medal_count, athletes) for medal_colour in (
  (gold_medals, gold_athletes) as 'GOLD',
  (silver_medals, silver_athletes) as 'SILVER',
  (bronze_medals, bronze_athletes) as 'BRONZE'
))
where  medal_colour = 'GOLD'
and    medal_count = 2
order  by noc
fetch first 3 rows only;

drop table olympic_medal_tables;
--13,
drop table if exists olympic_country_sport_medals;
create table olympic_country_sport_medals as
select * from (
  select noc, sport
  from   olympic_medal_winners
)
pivot (count(sport) for sport in (
  'Athletics' as ath, 'Artistic Gymnastics' as gym, 'Cycling Track' as cyc,
  'Boxing' as box, 'Sailing' as sai
  )
)
order  by 1;

--14,
select * from olympic_country_sport_medals
pivot  (
  sum(ath) ath, sum(box) box, sum(gym) gym, sum(sai) sai, sum(cyc) cyc
  for noc in (
    'BRA' BRA, 'CHN' CHN, 'DEN' DEN, 'ESP' ESP, 'ETH' ETH, 'GRE' GRE
  )
)
unpivot (
  ( BRA, CHN, DEN, ESP, ETH, GRE ) for sport in (
    (BRA_ATH, CHN_ATH, DEN_ATH,  ESP_ATH,  ETH_ATH, GRE_ATH) as 'Athletics',
    (BRA_GYM, CHN_GYM, DEN_GYM,  ESP_GYM,  ETH_GYM, GRE_GYM) as 'Artistic Gym',
    (BRA_BOX, CHN_BOX, DEN_BOX,  ESP_BOX,  ETH_BOX, GRE_BOX) as 'Boxing',
    (BRA_SAI, CHN_SAI, DEN_SAI,  ESP_SAI,  ETH_SAI, GRE_SAI) as 'Sailing',
    (BRA_CYC, CHN_CYC, DEN_CYC,  ESP_CYC,  ETH_CYC, GRE_CYC) as 'Track Cycling'
  )
);

--15,
select * from olympic_country_sport_medals
unpivot (
  (medals) for sport in ( ath, box, gym, sai, cyc )
)
pivot  (
  sum(medals) for noc in (
    'BRA' BRA, 'CHN' CHN, 'DEN' DEN, 'ESP' ESP, 'ETH' ETH, 'GRE' GRE
  )
);

--16, recursive grammar supports a three-operation chain
SELECT chained.*
FROM (
  olympic_country_sport_medals
  UNPIVOT (
    medals FOR sport IN (ath AS 'ATH', box AS 'BOX')
  )
  PIVOT (
    SUM(medals) FOR noc IN ('BRA' AS BRA, 'CHN' AS CHN)
  )
  UNPIVOT (
    medal_count FOR country IN (BRA AS 'BRA', CHN AS 'CHN')
  )
) chained
ORDER BY chained.sport, chained.country;

drop table olympic_country_sport_medals;
drop table olympic_medal_winners;

--17, Oracle 21c verified recursive PIVOT/UNPIVOT chain matrix
DROP TABLE IF EXISTS ora_chain_sales_verify;
DROP TABLE IF EXISTS ora_chain_wide_verify;

CREATE TABLE ora_chain_sales_verify
(
  product VARCHAR2(20),
  region  VARCHAR2(20),
  channel VARCHAR2(20),
  amount  NUMBER
);

INSERT INTO ora_chain_sales_verify VALUES ('Laptop', 'East', 'Online', 100);
INSERT INTO ora_chain_sales_verify VALUES ('Laptop', 'East', 'Store',   50);
INSERT INTO ora_chain_sales_verify VALUES ('Laptop', 'West', 'Online', 200);
INSERT INTO ora_chain_sales_verify VALUES ('Laptop', 'West', 'Store',   80);
INSERT INTO ora_chain_sales_verify VALUES ('Phone',  'East', 'Online', 300);
INSERT INTO ora_chain_sales_verify VALUES ('Phone',  'East', 'Store',   40);
INSERT INTO ora_chain_sales_verify VALUES ('Phone',  'West', 'Online',  20);
INSERT INTO ora_chain_sales_verify VALUES ('Phone',  'West', 'Store',  400);

CREATE TABLE ora_chain_wide_verify
(
  product   VARCHAR2(20),
  q1_amount NUMBER,
  q1_qty    NUMBER,
  q2_amount NUMBER,
  q2_qty    NUMBER
);

INSERT INTO ora_chain_wide_verify VALUES ('Laptop', 100, 2, 200, 3);
INSERT INTO ora_chain_wide_verify VALUES ('Phone',  300, 4, 400, 5);

-- T1: UNPIVOT -> PIVOT -> UNPIVOT
SELECT chain_upu.*
FROM ora_chain_wide_verify
UNPIVOT INCLUDE NULLS
(
  (amount, qty)
  FOR quarter_code IN
  (
    (q1_amount, q1_qty) AS 'Q1',
    (q2_amount, q2_qty) AS 'Q2'
  )
)
PIVOT
(
  SUM(amount) AS amount_total,
  SUM(qty)    AS qty_total
  FOR quarter_code IN
  (
    'Q1' AS q1_period,
    'Q2' AS q2_period
  )
)
UNPIVOT INCLUDE NULLS
(
  (amount, qty)
  FOR quarter_code IN
  (
    (q1_period_amount_total, q1_period_qty_total) AS 'Q1',
    (q2_period_amount_total, q2_period_qty_total) AS 'Q2'
  )
) chain_upu
ORDER BY chain_upu.product, chain_upu.quarter_code;

-- T2: PIVOT -> PIVOT
SELECT chain_pp.*
FROM ora_chain_sales_verify
PIVOT
(
  SUM(amount) AS amt
  FOR region IN
  (
    'East' AS east_r,
    'West' AS west_r
  )
)
PIVOT
(
  SUM(east_r_amt) AS east_total,
  SUM(west_r_amt) AS west_total
  FOR channel IN
  (
    'Online' AS online_ch,
    'Store'  AS store_ch
  )
) chain_pp
ORDER BY chain_pp.product;

-- T3: UNPIVOT -> UNPIVOT
SELECT chain_uu.*
FROM ora_chain_wide_verify
UNPIVOT INCLUDE NULLS
(
  (amount, qty)
  FOR quarter_code IN
  (
    (q1_amount, q1_qty) AS 'Q1',
    (q2_amount, q2_qty) AS 'Q2'
  )
)
UNPIVOT INCLUDE NULLS
(
  metric_value
  FOR metric_name IN
  (
    amount AS 'AMOUNT',
    qty    AS 'QTY'
  )
) chain_uu
ORDER BY chain_uu.product, chain_uu.quarter_code, chain_uu.metric_name;

-- T4: PIVOT -> UNPIVOT -> PIVOT -> UNPIVOT
SELECT chain_4.*
FROM ora_chain_sales_verify
PIVOT
(
  SUM(amount) AS amt
  FOR region IN ('East' AS east_r, 'West' AS west_r)
)
UNPIVOT INCLUDE NULLS
(
  amount FOR region IN
  (
    east_r_amt AS 'East',
    west_r_amt AS 'West'
  )
)
PIVOT
(
  SUM(amount) AS amt
  FOR channel IN ('Online' AS online_ch, 'Store' AS store_ch)
)
UNPIVOT INCLUDE NULLS
(
  amount FOR channel IN
  (
    online_ch_amt AS 'Online',
    store_ch_amt  AS 'Store'
  )
) chain_4
ORDER BY chain_4.product, chain_4.region, chain_4.channel;

-- T5: PIVOT -> UNPIVOT -> PIVOT -> UNPIVOT -> PIVOT
SELECT chain_5.*
FROM ora_chain_sales_verify
PIVOT
(
  SUM(amount) AS amt
  FOR region IN ('East' AS east_r, 'West' AS west_r)
)
UNPIVOT INCLUDE NULLS
(
  amount FOR region IN
  (
    east_r_amt AS 'East',
    west_r_amt AS 'West'
  )
)
PIVOT
(
  SUM(amount) AS amt
  FOR channel IN ('Online' AS online_ch, 'Store' AS store_ch)
)
UNPIVOT INCLUDE NULLS
(
  amount FOR channel IN
  (
    online_ch_amt AS 'Online',
    store_ch_amt  AS 'Store'
  )
)
PIVOT
(
  SUM(amount) AS amt
  FOR region IN ('East' AS east_final, 'West' AS west_final)
) chain_5
ORDER BY chain_5.product, chain_5.channel;

-- A parenthesized intermediate alias propagates through the outer operation.
SELECT mid_chain.*
FROM
(
  ora_chain_wide_verify
  UNPIVOT INCLUDE NULLS
  (
    amount FOR quarter_code IN
    (
      q1_amount AS 'Q1',
      q2_amount AS 'Q2'
    )
  )
) mid_chain
PIVOT
(
  SUM(amount) AS total
  FOR quarter_code IN ('Q1' AS q1_period, 'Q2' AS q2_period)
)
ORDER BY mid_chain.product, mid_chain.q1_qty, mid_chain.q2_qty;

DROP TABLE ora_chain_sales_verify;
DROP TABLE ora_chain_wide_verify;


drop table if exists h_athletes;
create table h_athletes (
    athlete_id   number primary key,
    athlete_name varchar2(50),
    noc          varchar2(3),
    gender       varchar2(1)
);

drop table if exists h_olympic_medals;
create table h_olympic_medals (
    medal_id     number primary key,
    athlete_id   number,
    sport        varchar2(50),
    event        varchar2(50),
    medal        varchar2(10), -- 'Gold', 'Silver', 'Bronze'
    year         number,
    foreign key (athlete_id) references h_athletes(athlete_id)
);

insert into h_athletes values (1, 'Alice', 'USA', 'F');
insert into h_athletes values (2, 'Bob', 'USA', 'M');
insert into h_athletes values (3, 'Charlie', 'CHN', 'M');
insert into h_athletes values (4, 'Diana', 'CHN', 'F');
insert into h_athletes values (5, 'Eve', 'BRA', 'F');

insert into h_olympic_medals values (1, 1, 'Athletics', '100m', 'Gold', 2020);
insert into h_olympic_medals values (2, 2, 'Athletics', '200m', 'Silver', 2020);
insert into h_olympic_medals values (3, 3, 'Swimming', '100m Freestyle', 'Gold', 2020);
insert into h_olympic_medals values (4, 4, 'Swimming', '200m Freestyle', 'Bronze', 2020);
insert into h_olympic_medals values (5, 5, 'Gymnastics', 'Vault', 'Silver', 2020);

select a.noc,
       a.athlete_name,
       a.gender,
       m.sport,
       m.event,
       m.medal
from h_athletes a
join h_olympic_medals m on a.athlete_id = m.athlete_id
order by a.noc, a.athlete_name;

--1, join + pivot
select *
from (
    select a.noc,
           m.sport,
           m.medal
    from h_athletes a
    join h_olympic_medals m on a.athlete_id = m.athlete_id
)
pivot (
    count(medal)
    for medal in ('Gold' as GOLD, 'Silver' as SILVER, 'Bronze' as BRONZE)
)
order by noc, sport;

--2, join + unpivot
select noc, sport, medal_type, count
from (
    select *
    from (
        select a.noc,
               m.sport,
               m.medal
        from h_athletes a
        join h_olympic_medals m on a.athlete_id = m.athlete_id
    )
    pivot (
        count(medal)
        for medal in ('Gold' as GOLD, 'Silver' as SILVER, 'Bronze' as BRONZE)
    )
)
unpivot (
    count for medal_type in (GOLD, SILVER, BRONZE)
)
order by noc, sport, medal_type;

--3,
select *
from (
    select gender, sport, medal
    from h_athletes a
    join h_olympic_medals m on a.athlete_id = m.athlete_id
)
pivot (
    count(medal)
    for gender in ('M' as MALE, 'F' as FEMALE)
);

drop table h_olympic_medals;
drop table h_athletes;

drop table if exists g_products;
CREATE TABLE g_products (
	product_id	 NUMBER PRIMARY KEY,
	product_name VARCHAR2(50)
);

INSERT INTO g_products VALUES (1, 'Laptop');
INSERT INTO g_products VALUES (2, 'Tablet');
INSERT INTO g_products VALUES (3, 'Phone');

drop table if exists g_regions;
CREATE TABLE g_regions (
    region_id NUMBER PRIMARY KEY,
    region_name VARCHAR2(50)
);

INSERT INTO g_regions VALUES (10, 'Americas');
INSERT INTO g_regions VALUES (20, 'Europe');
INSERT INTO g_regions VALUES (30, 'Asia');

drop table if exists g_sales;
CREATE TABLE g_sales (
    product_id NUMBER,
    region_id  NUMBER,
    q1 NUMBER,
    q2 NUMBER,
    q3 NUMBER,
    q4 NUMBER
);

INSERT INTO g_sales VALUES (1, 10, 100, 120, NULL, 150);
INSERT INTO g_sales VALUES (1, 20,  80, NULL,  90, 110);
INSERT INTO g_sales VALUES (2, 10, 200, 230, 210, NULL);
INSERT INTO g_sales VALUES (2, 30, NULL, NULL, 180, 200);
INSERT INTO g_sales VALUES (3, 20,  50,  70,  60,  80);

SELECT *
FROM (
    SELECT
        p.product_name,
        r.region_name,
        s.q1, s.q2, s.q3, s.q4
    FROM g_sales s
    JOIN g_products p ON s.product_id = p.product_id
    JOIN g_regions  r ON s.region_id  = r.region_id
)
PIVOT (
    SUM(q1) AS Q1,
    SUM(q2) AS Q2,
    SUM(q3) AS Q3,
    SUM(q4) AS Q4
    FOR region_name IN (
        'Americas' AS AMR,
        'Europe'   AS EUR,
        'Asia'     AS ASIA
    )
)
ORDER BY product_name;

SELECT *
FROM (
    SELECT
        p.product_name,
        r.region_name,
        s.q1, s.q2, s.q3, s.q4
    FROM g_sales s
    JOIN g_products p ON s.product_id = p.product_id
    JOIN g_regions  r ON s.region_id  = r.region_id
)
UNPIVOT (
    sales_amount FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2',
        q3 AS 'Q3',
        q4 AS 'Q4'
    )
)
ORDER BY product_name, region_name, quarter;

-- unpivot include nulls
SELECT *
FROM (
    SELECT
        p.product_name,
        r.region_name,
        s.q1, s.q2, s.q3, s.q4
    FROM g_sales s
    JOIN g_products p ON s.product_id = p.product_id
    JOIN g_regions  r ON s.region_id  = r.region_id
)
UNPIVOT INCLUDE NULLS
(
    sales_amount FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2',
        q3 AS 'Q3',
        q4 AS 'Q4'
    )
)
ORDER BY product_name, region_name, quarter;

-- unpivot exclude nulls
SELECT *
FROM (
    SELECT
        p.product_name,
        r.region_name,
        s.q1, s.q2, s.q3, s.q4
    FROM g_sales s
    JOIN g_products p ON s.product_id = p.product_id
    JOIN g_regions  r ON s.region_id  = r.region_id
)
UNPIVOT EXCLUDE NULLS
(
    sales_amount FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2',
        q3 AS 'Q3',
        q4 AS 'Q4'
    )
)
ORDER BY product_name, region_name, quarter;

WITH pivoted AS (
    SELECT *
    FROM (
        SELECT
            p.product_name,
            r.region_name,
            s.q1, s.q2, s.q3, s.q4
        FROM g_sales s
        JOIN g_products p ON s.product_id = p.product_id
        JOIN g_regions  r ON s.region_id  = r.region_id
    )
    PIVOT (
        SUM(q1 + q2 + q3 + q4) AS TOTAL
        FOR region_name IN ('Americas' AS AMR, 'Europe' AS EUR, 'Asia' AS ASIA)
    )
)
SELECT * FROM pivoted;

DROP TABLE g_sales;
DROP TABLE g_regions;
DROP TABLE g_products;

drop table if exists d_sales_quarterly;
CREATE TABLE d_sales_quarterly (
    product   VARCHAR2(20),
    q1_qty    NUMBER,
    q1_amt    NUMBER,
    q2_qty    NUMBER,
    q2_amt    NUMBER,
    q3_qty    NUMBER,
    q3_amt    NUMBER,
    q4_qty    NUMBER,
    q4_amt    NUMBER
);

INSERT INTO d_sales_quarterly VALUES ('A', 10, 100, 20, 220, 30, 330, 40, 480);
INSERT INTO d_sales_quarterly VALUES ('B',  5,  60, 15, 180, 25, 300, 35, 420);

SELECT product,
       quarter,
       qty,
       amt
FROM   d_sales_quarterly
UNPIVOT INCLUDE NULLS
(
    (qty, amt)
    FOR quarter IN (
        (q1_qty, q1_amt) AS 'Q1',
        (q2_qty, q2_amt) AS 'Q2',
        (q3_qty, q3_amt) AS 'Q3',
        (q4_qty, q4_amt) AS 'Q4'
    )
)
ORDER BY product, quarter;

DROP TABLE d_sales_quarterly;

drop table if exists sales_pu;
CREATE TABLE sales_pu (
    product_name VARCHAR2(30),
    region_name  VARCHAR2(30),
    year_no      NUMBER,
    quarter      VARCHAR2(2),
    sales_amount NUMBER,
    sales_qty    NUMBER
);

INSERT INTO sales_pu VALUES ('Laptop', 'Americas', 2024, 'Q1', 100, 10);
INSERT INTO sales_pu VALUES ('Laptop', 'Americas', 2024, 'Q2', 120, 12);
INSERT INTO sales_pu VALUES ('Laptop', 'Europe',   2024, 'Q1',  80,  8);
INSERT INTO sales_pu VALUES ('Laptop', 'Europe',   2024, 'Q2', NULL, 9);
INSERT INTO sales_pu VALUES ('Tablet', 'Americas', 2024, 'Q1', 200, 20);
INSERT INTO sales_pu VALUES ('Tablet', 'Americas', 2024, 'Q2', 230, 23);
INSERT INTO sales_pu VALUES ('Tablet', 'Asia',     2025, 'Q1', 180, 18);
INSERT INTO sales_pu VALUES ('Tablet', 'Asia',     2025, 'Q2', 200, 20);
INSERT INTO sales_pu VALUES ('Phone',  'Europe',   2025, 'Q1',  50,  5);
INSERT INTO sales_pu VALUES ('Phone',  'Europe',   2025, 'Q2',  70,  7);

-- ============================
-- 1. PIVOT
--   aggregate_function(expr)
--   FOR column
--   IN (expr, expr, ...)
-- ============================

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount)
    FOR quarter IN ('Q1', 'Q2')
)
ORDER BY product_name;

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
) su
PIVOT (
    SUM(su.sales_amount)
    FOR quarter IN ('Q1', 'Q2')
)
ORDER BY product_name;

SELECT *
    FROM sales_pu su
PIVOT (
    SUM(su.sales_amount)
    FOR quarter IN ('Q1', 'Q2')
)
ORDER BY product_name;

-- ====================================
-- 2. PIVOT
--   aggregate_function(expr) AS alias
--   expr AS alias
-- ====================================

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS amt
    FOR quarter IN (
        'Q1' AS q1,
        'Q2' AS q2
    )
)
ORDER BY product_name;


-- ====================================
-- 3. PIVOT
--   aggregate_function(expr) alias,
--   aggregate_function(expr) alias ...
-- ====================================

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount, sales_qty
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS sum_amt,
    COUNT(sales_amount) AS cnt_amt,
    MAX(sales_qty) AS max_qty
    FOR quarter IN (
        'Q1' AS q1,
        'Q2' AS q2
    )
)
ORDER BY product_name;


-- ==================================
-- 4. PIVOT
--   FOR (column, column, ...)
--   IN ((expr, expr) AS alias, ...)
-- ==================================

SELECT *
FROM (
    SELECT product_name, year_no, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS amt
    FOR (year_no, quarter) IN (
        (2024, 'Q1') AS y2024_q1,
        (2024, 'Q2') AS y2024_q2,
        (2025, 'Q1') AS y2025_q1,
        (2025, 'Q2') AS y2025_q2
    )
)
ORDER BY product_name;


-- =====================================
-- 5. PIVOT
--   pivot_in_clause  (expr [, expr]...)
--   single expression list
-- =====================================

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS amt
    FOR quarter IN (
        ('Q1') AS q1,
        ('Q2') AS q2
    )
)
ORDER BY product_name;


-- ===============
-- 6. PIVOT
--   [[AS] alias]
-- ===============

SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) amt
    FOR quarter IN (
        'Q1' q1,
        'Q2' q2
    )
)
ORDER BY product_name;

-- Generated names must be unique after aggregate aliases are appended
SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS amt
    FOR quarter IN (
        'Q1' AS duplicate_name,
        'Q2' AS duplicate_name
    )
);

-- Generated names must not be silently truncated to NAMEDATALEN
SELECT *
FROM (
    SELECT product_name, quarter, sales_amount
    FROM sales_pu
)
PIVOT (
    SUM(sales_amount) AS this_is_a_long_aggregate_alias
    FOR quarter IN (
        'Q1' AS this_is_a_very_long_pivot_value_alias_1234567890
    )
);


-- ==============================
-- 7. UNPIVOT
--   UNPIVOT
--   column
--   FOR column
--   IN (column AS literal, ...)
-- default is EXCLUDE NULLS
-- ==============================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT (
    sales_amount
    FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2'
    )
)
ORDER BY product_name, region_name, quarter;


-- ========================
-- 8. UNPIVOT
--   UNPIVOT EXCLUDE NULLS
-- ========================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT EXCLUDE NULLS (
    sales_amount
    FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2'
    )
)
ORDER BY product_name, region_name, quarter;


-- ========================
-- 9. UNPIVOT
--   UNPIVOT INCLUDE NULLS
-- ========================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT INCLUDE NULLS (
    sales_amount
    FOR quarter IN (
        q1 AS 'Q1',
        q2 AS 'Q2'
    )
)
ORDER BY product_name, region_name, quarter;


-- ========================
-- 10. UNPIVOT
--   unpivot_in_clause:
--     column
--     column, column ...
-- ========================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT (
    sales_amount
    FOR quarter IN (
        q1,
        q2
    )
)
ORDER BY product_name, region_name, quarter;


-- ========================================
-- 11. UNPIVOT
--   UNPIVOT (column, column, ...)
--   IN ((column, column) AS literal, ...)
-- ========================================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1_amt,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_qty    END) AS q1_qty,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2_amt,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_qty    END) AS q2_qty
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT (
    (sales_amount, sales_qty)
    FOR quarter IN (
        (q1_amt, q1_qty) AS 'Q1',
        (q2_amt, q2_qty) AS 'Q2'
    )
)
ORDER BY product_name, region_name, quarter;


-- ============================================
-- 12. UNPIVOT
--   AS { literal | (literal [, literal]...) }
--
--     FOR (column, column, ...)
-- ============================================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN year_no = 2024 AND quarter = 'Q1' THEN sales_amount END) AS y2024_q1_amt,
           SUM(CASE WHEN year_no = 2024 AND quarter = 'Q1' THEN sales_qty    END) AS y2024_q1_qty,
           SUM(CASE WHEN year_no = 2024 AND quarter = 'Q2' THEN sales_amount END) AS y2024_q2_amt,
           SUM(CASE WHEN year_no = 2024 AND quarter = 'Q2' THEN sales_qty    END) AS y2024_q2_qty,
           SUM(CASE WHEN year_no = 2025 AND quarter = 'Q1' THEN sales_amount END) AS y2025_q1_amt,
           SUM(CASE WHEN year_no = 2025 AND quarter = 'Q1' THEN sales_qty    END) AS y2025_q1_qty,
           SUM(CASE WHEN year_no = 2025 AND quarter = 'Q2' THEN sales_amount END) AS y2025_q2_amt,
           SUM(CASE WHEN year_no = 2025 AND quarter = 'Q2' THEN sales_qty    END) AS y2025_q2_qty
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT INCLUDE NULLS (
    (sales_amount, sales_qty)
    FOR (year_no, quarter) IN (
        (y2024_q1_amt, y2024_q1_qty) AS (2024, 'Q1'),
        (y2024_q2_amt, y2024_q2_qty) AS (2024, 'Q2'),
        (y2025_q1_amt, y2025_q1_qty) AS (2025, 'Q1'),
        (y2025_q2_amt, y2025_q2_qty) AS (2025, 'Q2')
    )
)
ORDER BY product_name, region_name, year_no, quarter;


-- =========================
-- 13. UNPIVOT
--   (column [, column]...)
-- =========================

SELECT *
FROM (
    SELECT product_name, region_name,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_amount END) AS q1_amt,
           SUM(CASE WHEN quarter = 'Q1' THEN sales_qty    END) AS q1_qty,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_amount END) AS q2_amt,
           SUM(CASE WHEN quarter = 'Q2' THEN sales_qty    END) AS q2_qty
    FROM sales_pu
    GROUP BY product_name, region_name
)
UNPIVOT (
    (sales_amount, sales_qty)
    FOR quarter IN (
        (q1_amt, q1_qty),
        (q2_amt, q2_qty)
    )
)
ORDER BY product_name, region_name, quarter;

drop table sales_pu;

DROP TABLE if exists piv_diff_test;
CREATE TABLE piv_diff_test (
  order_id NUMBER,
  product_id NUMBER,
  product_name VARCHAR2(20),
  amount NUMBER
);

INSERT INTO piv_diff_test VALUES (1, 10, 'APPLE', 100);
INSERT INTO piv_diff_test VALUES (2, 20, 'ORANGE', 200);
INSERT INTO piv_diff_test VALUES (3, 10, 'APPLE', 300);

--1. PIVOT IN supports constant expressions, 10 + 0
SELECT *
FROM piv_diff_test
PIVOT (
  SUM(amount)
  FOR product_id IN (
    10 + 0 AS p10,
    20     AS p20
  )
);

-- negative numbers
SELECT *
FROM piv_diff_test
PIVOT (
  SUM(amount)
  FOR product_id IN (
    -10 AS neg10,
    20  AS p20
  )
);

--2. When multiple aggregate functions are used in PIVOT, at most one agregate expression may omit an alias
SELECT *
FROM piv_diff_test
PIVOT (
  SUM(amount) total_amount,
  COUNT(*)
  FOR product_name IN (
    'APPLE'  AS apple,
    'ORANGE' AS orange
  )
);

--4. Aggregation parameters are complex expressions, such as CASE...WHEN
SELECT *
FROM piv_diff_test
PIVOT (
  SUM(CASE WHEN amount > 0 THEN amount ELSE 0 END)
  FOR product_name IN (
    'APPLE'  AS apple,
    'ORANGE' AS orange
  )
);

DROP TABLE piv_diff_test;

DROP TABLE if exists expr_test_data;
CREATE TABLE expr_test_data (
    product_name   VARCHAR2(50),
    store_id       NUMBER,
    business_date  DATE,
    revenue        NUMBER(12,2)
);

INSERT INTO expr_test_data VALUES ('Laptop', 10, TO_DATE('2026-01-01', 'YYYY-MM-DD'), 1500.00);
INSERT INTO expr_test_data VALUES ('Laptop', 20, TO_DATE('2026-05-15', 'YYYY-MM-DD'), 1600.00);
INSERT INTO expr_test_data VALUES ('Phone',  10, TO_DATE('2026-01-01', 'YYYY-MM-DD'), 800.00);
INSERT INTO expr_test_data VALUES ('Phone',  30, TO_DATE('2026-12-31', 'YYYY-MM-DD'), 950.00);

SELECT * FROM (
    SELECT store_id, product_name, revenue FROM expr_test_data
)
PIVOT (
    SUM(revenue)
    FOR (store_id, product_name) IN (
        (10, 'Laptop')          AS store10_laptop,
        (20, 'Laptop'),
        (30, UPPER('p')||'hone') AS store30_phone
    )
);

SELECT * FROM (
    SELECT business_date, store_id, revenue FROM expr_test_data
)
PIVOT (
    SUM(revenue)
    FOR (business_date, store_id) IN (
        (TO_DATE('2026-01-01', 'YYYY-MM-DD'), 10),
        (TRUNC(TO_DATE('2026-05-20', 'YYYY-MM-DD'), 'MM'), 10 + 10) AS mid_year_store20
    )
);

SELECT * FROM (
    SELECT product_name, store_id, revenue FROM expr_test_data
)
PIVOT (
    SUM(revenue)
    FOR product_name IN (
        'Laptop',
        revenue
    )
);

DROP TABLE expr_test_data;

DROP TABLE if exists pivot_expr_test;
CREATE TABLE pivot_expr_test (
    id      NUMBER,
    k_num   NUMBER,
    k_chr   VARCHAR2(30),
    k_date  DATE,
    k_ts    TIMESTAMP,
    amount  NUMBER
);

INSERT INTO pivot_expr_test VALUES (
    1, 1, 'A',
    DATE '2024-01-01',
    TIMESTAMP '2024-01-01 10:00:00',
    100
);

INSERT INTO pivot_expr_test VALUES (
    1, 2, 'B',
    DATE '2024-01-02',
    TIMESTAMP '2024-01-02 10:00:00',
    200
);

INSERT INTO pivot_expr_test VALUES (
    2, 3, 'C',
    DATE '2024-01-03',
    TIMESTAMP '2024-01-03 10:00:00',
    300
);

INSERT INTO pivot_expr_test VALUES (
    2, NULL, NULL,
    NULL,
    NULL,
    400
);

--
--1 Numeric constant expr
--
-- numceri literal
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        1 AS n1,
        2 AS n2,
        3 AS n3
    )
)
ORDER BY id;

-- plus-minus sign
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        +1 AS plus_1,
        -1 AS minus_1,
        2 AS n2
    )
)
ORDER BY id;

-- Numerical arithmetic expression
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        1 + 0 AS expr_1,
        1 * 2 AS expr_2,
        10 - 7 AS expr_3
    )
)
ORDER BY id;

-- Numerical function expression
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        ABS(-1) AS abs_1,
        ROUND(1.6) AS round_2,
        TRUNC(3.9) AS trunc_3
    )
)
ORDER BY id;

--
-- 2, String constant expr
--

-- String literal
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        'A' AS a,
        'B' AS b,
        'C' AS c
    )
)
ORDER BY id;

-- String concatenation expression
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        'A' || '' AS a_expr,
        'B' || '' AS b_expr,
        'C' || '' AS c_expr
    )
)
ORDER BY id;

-- String function expression
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        UPPER('a') AS a_upper,
        LOWER('B') AS b_lower,
        SUBSTR('CCC', 1, 1) AS c_substr
    )
)
ORDER BY id;


-- q-quoted string
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        q'[A]' AS q_a,
        q'[B]' AS q_b,
        q'[C]' AS q_c
    )
)
ORDER BY id;

--
-- 3, DATE literal
--

-- date literal
SELECT *
FROM (
    SELECT id, k_date, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_date IN (
        DATE '2024-01-01' AS d_20240101,
        DATE '2024-01-02' AS d_20240102,
        DATE '2024-01-03' AS d_20240103
    )
)
ORDER BY id;

-- date arithmetic expression
SELECT *
FROM (
    SELECT id, k_date, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_date IN (
        DATE '2023-12-31' + 1 AS d_20240101,
        DATE '2024-01-01' + 1 AS d_20240102,
        DATE '2024-01-01' + 2 AS d_20240103
    )
)
ORDER BY id;

-- to_date constant expression
SELECT *
FROM (
    SELECT id, k_date, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_date IN (
        TO_DATE('2024-01-01', 'YYYY-MM-DD') AS d1,
        TO_DATE('2024-01-02', 'YYYY-MM-DD') AS d2,
        TO_DATE('2024-01-03', 'YYYY-MM-DD') AS d3
    )
)
ORDER BY id;

--
-- 4, TIMESTAMP literal
--

-- timestamp literal
SELECT *
FROM (
    SELECT id, k_ts, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_ts IN (
        TIMESTAMP '2024-01-01 10:00:00' AS ts1,
        TIMESTAMP '2024-01-02 10:00:00' AS ts2,
        TIMESTAMP '2024-01-03 10:00:00' AS ts3
    )
)
ORDER BY id;

-- timestamp + interval expression
SELECT *
FROM (
    SELECT id, k_ts, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_ts IN (
        TIMESTAMP '2024-01-01 09:00:00' + INTERVAL '1' HOUR AS ts1,
        TIMESTAMP '2024-01-02 09:00:00' + INTERVAL '1' HOUR AS ts2,
        TIMESTAMP '2024-01-03 09:00:00' + INTERVAL '1' HOUR AS ts3
    )
)
ORDER BY id;

-- to_timestamp constant expression
SELECT *
FROM (
    SELECT id, k_ts, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_ts IN (
        TO_TIMESTAMP('2024-01-01 10:00:00', 'YYYY-MM-DD HH24:MI:SS') AS ts1,
        TO_TIMESTAMP('2024-01-02 10:00:00', 'YYYY-MM-DD HH24:MI:SS') AS ts2,
        TO_TIMESTAMP('2024-01-03 10:00:00', 'YYYY-MM-DD HH24:MI:SS') AS ts3
    )
)
ORDER BY id;

--
-- 5, NULL as pivot expr
--

-- null constant
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        1 AS n1,
        2 AS n2,
        NULL AS null_key
    )
)
ORDER BY id;

-- CAST(NULL as type)
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        'A' AS a,
        CAST(NULL AS VARCHAR2(30)) AS null_chr
    )
)
ORDER BY id;

--
-- 6, CAST / Type conversion expr
--

-- CAST numerical
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        CAST(1 AS NUMBER) AS n1,
        CAST(2 AS NUMBER) AS n2,
        CAST(3 AS NUMBER) AS n3
    )
)
ORDER BY id;

-- CAST character
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        CAST('A' AS VARCHAR2(30)) AS a,
        CAST('B' AS VARCHAR2(30)) AS b,
        CAST('C' AS VARCHAR2(30)) AS c
    )
)
ORDER BY id;

-- CAST DATE / TIMESTAMP
SELECT *
FROM (
    SELECT id, k_date, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_date IN (
        CAST(DATE '2024-01-01' AS DATE) AS d1,
        CAST(DATE '2024-01-02' AS DATE) AS d2
    )
)
ORDER BY id;


--
-- 7, Constant expressions such as CASE / DECODE / NVL
--

-- CASE constant expression
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        CASE WHEN 1 = 1 THEN 'A' ELSE 'X' END AS case_a,
        CASE WHEN 2 = 2 THEN 'B' ELSE 'X' END AS case_b
    )
)
ORDER BY id;

-- decode constant expression
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        DECODE(1, 1, 'A', 'X') AS decode_a,
        DECODE(2, 2, 'B', 'X') AS decode_b
    )
)
ORDER BY id;

-- NVL / COALESCE constant expression
SELECT *
FROM (
    SELECT id, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_chr IN (
        NVL(NULL, 'A') AS nvl_a,
        COALESCE(NULL, 'B') AS coalesce_b
    )
)
ORDER BY id;

-- Multi-column FOR: each IN item is a list of expression
SELECT *
FROM (
    SELECT id, k_num, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR (k_num, k_chr) IN (
        (1, 'A') AS n1_a,
        (2, 'B') AS n2_b,
        (1 + 2, UPPER('c')) AS n3_c
    )
)
ORDER BY id;

-- Multi-column FOR: DATE + String
SELECT *
FROM (
    SELECT id, k_date, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR (k_date, k_chr) IN (
        (DATE '2024-01-01', 'A') AS d1_a,
        (DATE '2024-01-02', 'B') AS d2_b,
        (DATE '2024-01-03', 'C') AS d3_c
    )
)
ORDER BY id;

-- Cannot use the pivot column itself or other input columns as IN expr
SELECT *
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (
        k_num AS bad_expr
    )
);

-- Alias following the PIVOT clause names the transformed table reference
SELECT p.id, p.n1
FROM (
    SELECT id, k_num, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR k_num IN (1 AS n1)
) p
ORDER BY p.id;

-- Each IN item must supply one value for every FOR column
SELECT *
FROM (
    SELECT id, k_num, k_chr, amount
    FROM pivot_expr_test
)
PIVOT (
    SUM(amount)
    FOR (k_num, k_chr) IN (1 AS bad_count)
);

DROP TABLE pivot_expr_test;

--
-- Support PIVOT XML
--
DROP TABLE if exists pivot_xml_sales;

CREATE TABLE pivot_xml_sales (
    region       VARCHAR2(10),
    salesman    VARCHAR2(20),
    product     VARCHAR2(20),
    amount      NUMBER,
    order_count NUMBER
);

INSERT INTO pivot_xml_sales VALUES ('N', 'Alice', 'Apple',  100, 1);
INSERT INTO pivot_xml_sales VALUES ('N', 'Alice', 'Apple',  100, 1);
INSERT INTO pivot_xml_sales VALUES ('N', 'Alice', 'Apple',  120, 1);
INSERT INTO pivot_xml_sales VALUES ('N', 'Alice', 'Orange', 200, 1);
INSERT INTO pivot_xml_sales VALUES ('S', 'Bob',   'Apple',   80, 1);
INSERT INTO pivot_xml_sales VALUES ('S', 'Bob',   'Berry',  300, 1);
INSERT INTO pivot_xml_sales VALUES  ('S', 'Bob',   'Berry',  320, 1);

--1, PIVOT XML IN (ANY) includes only values present in each group
SELECT region, salesman
FROM (
    SELECT region, salesman, product, amount, order_count
    FROM pivot_xml_sales
)
PIVOT XML (
    SUM(amount) AS total,
    COUNT(*) AS rows_seen
    FOR product IN (ANY)
)
ORDER BY region, salesman;

--2, PIVOT XML IN (subquery) includes all subquery values for every group
SELECT region, salesman
FROM (
    SELECT region, salesman, product, amount
    FROM pivot_xml_sales
)
PIVOT XML (
    SUM(amount) AS total
    FOR product IN (SELECT DISTINCT product FROM pivot_xml_sales)
)
ORDER BY region, salesman;

--3, PIVOT XML count(*) returns zero for subquery values with no matching rows
SELECT *
FROM (
    SELECT region, salesman, product
    FROM pivot_xml_sales
)
PIVOT XML (
    COUNT(*) AS cnt
    FOR product IN (SELECT DISTINCT product FROM pivot_xml_sales)
)
ORDER BY region, salesman;

--4, COUNT(*) uses a non-null source-row marker when the FOR value is NULL
DROP TABLE if exists pivot_xml_null_key;
CREATE TABLE pivot_xml_null_key (
    grp     VARCHAR2(10),
    key_col VARCHAR2(10),
    amount  NUMBER
);
INSERT INTO pivot_xml_null_key VALUES ('A', NULL, 10);
INSERT INTO pivot_xml_null_key VALUES ('A', NULL, 20);

SELECT *
FROM pivot_xml_null_key
PIVOT XML (
    COUNT(*) AS cnt,
    SUM(amount) AS total
    FOR key_col IN (ANY)
)
ORDER BY grp;

DROP TABLE pivot_xml_null_key;

DROP TABLE pivot_xml_sales;

--5, PIVOT XML multi-column FOR with matching ANY list
DROP TABLE if exists pivot_xml_multi_for;

CREATE TABLE pivot_xml_multi_for (
    product VARCHAR2(20),
    region  VARCHAR2(20),
    channel VARCHAR2(20),
    amount  NUMBER
);

INSERT INTO pivot_xml_multi_for VALUES ('Laptop', 'East', 'Online', 100);
INSERT INTO pivot_xml_multi_for VALUES ('Laptop', 'East', 'Online',  50);
INSERT INTO pivot_xml_multi_for VALUES ('Laptop', 'East', 'Store',  150);
INSERT INTO pivot_xml_multi_for VALUES ('Laptop', 'West', 'Online', 200);
INSERT INTO pivot_xml_multi_for VALUES ('Phone',  'East', 'Online', 300);
INSERT INTO pivot_xml_multi_for VALUES ('Phone',  'West', 'Store',  400);

SELECT p.product, p."region_channel_xml"
FROM pivot_xml_multi_for
PIVOT XML (
    SUM(amount) AS total_amount
    FOR (region, channel) IN (ANY, ANY)
) p
ORDER BY product;

--6, duplicate multi-column subquery combinations are rejected at runtime
SELECT p.product, p."region_channel_xml"
FROM pivot_xml_multi_for
PIVOT XML (
    SUM(amount) AS total_amount
    FOR (region, channel) IN (
        SELECT region, channel FROM pivot_xml_multi_for
    )
) p
ORDER BY product;

--7, multi-column subquery values are densified across implicit groups
SELECT p.product, p."region_channel_xml"
FROM pivot_xml_multi_for
PIVOT XML (
    SUM(amount) AS total_amount
    FOR (region, channel) IN (
        SELECT DISTINCT region, channel FROM pivot_xml_multi_for
    )
) p
ORDER BY product;

--8, the number of ANY keywords must match the number of FOR columns
SELECT p.*
FROM pivot_xml_multi_for
PIVOT XML (
    SUM(amount) AS total_amount
    FOR (region, channel) IN (ANY)
) p;

DROP TABLE pivot_xml_multi_for;

--9, subquery keys are coerced to the FOR column type before comparison
DROP TABLE if exists pivot_xml_typed_key;
CREATE TABLE pivot_xml_typed_key (
    grp     VARCHAR2(10),
    key_col CHAR(3),
    amount  NUMBER
);
INSERT INTO pivot_xml_typed_key VALUES ('A', 'X', 10);

SELECT p.*
FROM pivot_xml_typed_key
PIVOT XML (
    SUM(amount) AS total
    FOR key_col IN (
        SELECT CAST('X' AS VARCHAR2(3)) FROM dual
    )
) p;

DROP TABLE pivot_xml_typed_key;

--10, XML name attributes retain spelling; the generated output column is lowercase
DROP TABLE if exists pivot_xml_identifier_test;
CREATE TABLE pivot_xml_identifier_test (
    "Group Key"     VARCHAR2(10),
    "lowercase"     VARCHAR2(20),
    "Pivot&Key"     VARCHAR2(20),
    "Measure&Value" NUMBER
);
INSERT INTO pivot_xml_identifier_test
VALUES ('G1', 'A&B', '<Online>', 12.50);
INSERT INTO pivot_xml_identifier_test
VALUES ('G1', NULL, NULL, 7);

SELECT p."Group Key", p."lowercase_pivot&key_xml"
FROM pivot_xml_identifier_test
PIVOT XML (
    SUM("Measure&Value") AS "Total&Value"
    FOR ("lowercase", "Pivot&Key") IN (ANY, ANY)
) p
ORDER BY p."Group Key";

-- Ordinary PIVOT combines quoted aliases without folding or losing symbols
SELECT p.*
FROM pivot_xml_identifier_test
PIVOT (
    SUM("Measure&Value") AS "Total&Value"
    FOR ("lowercase", "Pivot&Key") IN (
        ('A&B', '<Online>') AS "Mixed Alias"
    )
) p;

-- Generated XML output names must not be silently truncated
SELECT p.*
FROM (
    SELECT "Group Key",
           "lowercase" AS
             "this_is_a_quoted_for_column_name_that_is_long_enough_1234567",
           "Measure&Value"
    FROM pivot_xml_identifier_test
)
PIVOT XML (
    SUM("Measure&Value") AS total
    FOR "this_is_a_quoted_for_column_name_that_is_long_enough_1234567"
    IN (ANY)
) p;

DROP TABLE pivot_xml_identifier_test;

--11, Oracle DATE/TIMESTAMP/NUMBER retain their native XML output forms
DROP TABLE if exists pivot_xml_scalar_format_test;
CREATE TABLE pivot_xml_scalar_format_test (
    grp          VARCHAR2(10),
    date_key     DATE,
    ts_measure   TIMESTAMP,
    amount       NUMBER(10,4),
    null_measure NUMBER
);
INSERT INTO pivot_xml_scalar_format_test VALUES (
    'G1', DATE '2024-01-02',
    TIMESTAMP '2024-01-02 03:04:05.123456',
    123.4500, NULL
);

SELECT p.*
FROM pivot_xml_scalar_format_test
PIVOT XML (
    MAX(ts_measure) AS max_ts,
    SUM(amount) AS total_amount,
    MAX(null_measure) AS null_measure
    FOR date_key IN (ANY)
) p;

DROP TABLE pivot_xml_scalar_format_test;

-- Oracle DATE time fields reach XMLELEMENT without a cast to pg_catalog.date.
SET nls_date_format = 'YYYY-MM-DD HH24:MI:SS';
SELECT p.*
FROM (
    SELECT TO_DATE('2024-01-02 03:04:05', 'YYYY-MM-DD HH24:MI:SS') AS date_key,
           1 AS amount
    FROM dual
)
PIVOT XML (SUM(amount) AS total FOR date_key IN (ANY)) p;
RESET nls_date_format;


drop table if exists cust_sales_category;
CREATE TABLE cust_sales_category (
    location      VARCHAR2(50),
    prod_category VARCHAR2(50),
    customer_id   NUMBER,
    sale_amount   NUMBER
);

INSERT INTO cust_sales_category VALUES ('north', 'furniture', 2, 875);
INSERT INTO cust_sales_category VALUES ('south', 'electronics', 2, 378);
INSERT INTO cust_sales_category VALUES ('east', 'gardening', 4, 136);
INSERT INTO cust_sales_category VALUES ('west', 'electronics', 3, 236);
INSERT INTO cust_sales_category VALUES ('central', 'furniture', 3, 174);
INSERT INTO cust_sales_category VALUES ('north', 'electronics', 1, 729);
INSERT INTO cust_sales_category VALUES ('east', 'gardening', 2, 147);
INSERT INTO cust_sales_category VALUES ('west', 'electronics', 3, 200);
INSERT INTO cust_sales_category VALUES ('north', 'furniture', 4, 987);
INSERT INTO cust_sales_category VALUES ('central', 'gardening', 4, 584);
INSERT INTO cust_sales_category VALUES ('south', 'electronics', 3, 714);
INSERT INTO cust_sales_category VALUES ('east', 'furniture', 1, 192);
INSERT INTO cust_sales_category VALUES ('west', 'gardening', 3, 946);
INSERT INTO cust_sales_category VALUES ('east', 'electronics', 4, 649);
INSERT INTO cust_sales_category VALUES ('south', 'furniture', 2, 503);
INSERT INTO cust_sales_category VALUES ('north', 'electronics', 1, 399);
INSERT INTO cust_sales_category VALUES ('central', 'gardening', 3, 259);
INSERT INTO cust_sales_category VALUES ('east', 'electronics', 3, 407);
INSERT INTO cust_sales_category VALUES ('west', 'furniture', 1, 545);

SELECT *
FROM (
  SELECT location, customer_id, sale_amount
  FROM cust_sales_category
)
PIVOT XML (
  SUM(sale_amount) AS sales_total
  FOR customer_id
  IN (1, 2, 3, 4)
);

-- PIVOT XML ANY
SELECT *
FROM (
  SELECT location, customer_id, sale_amount
  FROM cust_sales_category
)
PIVOT XML (
  SUM(sale_amount) AS sales_total
  FOR customer_id
  IN (ANY)
);

-- PIVOT XML subquery must reject duplicate runtime pivot values
SELECT *
FROM (
  SELECT location, customer_id, sale_amount
  FROM cust_sales_category
)
PIVOT XML (
  SUM(sale_amount) AS sales_total
  FOR customer_id
  IN (
    SELECT customer_id
    FROM cust_sales_category
  )
);

-- A unique subquery value set is densified across every implicit group
SELECT *
FROM (
  SELECT location, customer_id, sale_amount
  FROM cust_sales_category
)
PIVOT XML (
  SUM(sale_amount) AS sales_total
  FOR customer_id
  IN (
    SELECT DISTINCT customer_id
    FROM cust_sales_category
    WHERE customer_id <= 3
  )
);

drop table cust_sales_category;

-- UNPIVOT applies Oracle character and numeric datatype-group precedence
DROP TABLE if exists unpivot_type_group_test;
CREATE TABLE unpivot_type_group_test (
    id       NUMBER,
    c_fixed  CHAR(4),
    c_fixed2 CHAR(6),
    c_var    VARCHAR2(8),
    n_number NUMBER,
    n_float  BINARY_FLOAT,
    n_double BINARY_DOUBLE,
    d1       DATE,
    d2       DATE
);

INSERT INTO unpivot_type_group_test VALUES (
    1, 'A', 'C', 'B', 10,
    CAST(20 AS BINARY_FLOAT),
    CAST(30 AS BINARY_DOUBLE),
    DATE '2024-01-01', DATE '2024-01-02'
);

SELECT kind, val, pg_typeof(val) AS value_type
FROM (
    SELECT id, c_fixed, c_var
    FROM unpivot_type_group_test
)
UNPIVOT (
    val FOR kind IN (c_fixed AS 'FIXED', c_var AS 'VARYING')
)
ORDER BY kind;

SELECT kind, val, pg_typeof(val) AS value_type
FROM (
    SELECT id, c_fixed, c_fixed2
    FROM unpivot_type_group_test
)
UNPIVOT (
    val FOR kind IN (c_fixed AS 'CHAR4', c_fixed2 AS 'CHAR6')
)
ORDER BY kind;

SELECT kind, val, pg_typeof(val) AS value_type
FROM (
    SELECT id, n_number, n_float, n_double
    FROM unpivot_type_group_test
)
UNPIVOT (
    val FOR kind IN (
        n_number AS 'NUMBER',
        n_float AS 'BINARY_FLOAT',
        n_double AS 'BINARY_DOUBLE'
    )
)
ORDER BY kind;

SELECT kind, char_val, number_val,
       pg_typeof(char_val) AS char_type,
       pg_typeof(number_val) AS number_type
FROM (
    SELECT id, c_fixed, c_var, n_number, n_float
    FROM unpivot_type_group_test
)
UNPIVOT (
    (char_val, number_val) FOR kind IN (
        (c_fixed, n_number) AS 'A',
        (c_var, n_float) AS 'B'
    )
)
ORDER BY kind;

SELECT kind, val, pg_typeof(val) AS value_type
FROM (
    SELECT id, d1, d2
    FROM unpivot_type_group_test
)
UNPIVOT (
    val FOR kind IN (d1 AS 'D1', d2 AS 'D2')
)
ORDER BY kind;

-- Character and numeric source columns are not in one datatype group
SELECT *
FROM (
    SELECT id, c_var, n_number
    FROM unpivot_type_group_test
)
UNPIVOT (
    val FOR kind IN (c_var AS 'CHAR', n_number AS 'NUMBER')
);

DROP TABLE unpivot_type_group_test;
