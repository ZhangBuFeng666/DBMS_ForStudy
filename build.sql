--grant select on V_$session to c##scott;
DROP TABLE student CASCADE CONSTRAINTS;
DROP TABLE class ;
DROP TABLE sc CASCADE CONSTRAINTS;
DROP TABLE teaching CASCADE CONSTRAINTS;
DROP TABLE teacher;
DROP TABLE course ;
CREATE TABLE class(
       classno VARCHAR2(6) PRIMARY KEY,
       classname VARCHAR(20) NOT NULL,
       classmajor VARCHAR2(20),
       classdept VARCHAR2(20),
       studentnumber SMALLINT CHECK(studentnumber BETWEEN 20 AND 40)

       );
CREATE TABLE student(
       sno VARCHAR2(8) PRIMARY KEY,
       sname VARCHAR2(8) NOT NULL,
       ssex VARCHAR2(2) CHECK(ssex IN ('ÄÐ','Å®')),
       sbirthday DATE DEFAULT SYSDATE,
       classno VARCHAR2(6),
       Totalcredit SMALLINT DEFAULT 0,
       CONSTRAINT constrint_classno FOREIGN KEY (classno) REFERENCES class(classno)
       );
CREATE TABLE course(
       cno VARCHAR2(6) PRIMARY KEY,
       cname VARCHAR(30) NOT NULL,
       ccredit SMALLINT CHECK(ccredit BETWEEN 1 AND 4)
       );
CREATE TABLE sc(
       sno VARCHAR(8),
       cno VARCHAR(6),
       grade INTEGER CHECK(grade BETWEEN 0 AND 100)
       );
CREATE TABLE teacher(
       tno NUMBER(6) PRIMARY KEY,
       tname VARCHAR2(8) NOT NULL,
       tsex VARCHAR2(2) CHECK(tsex IN ('ÄÐ','Å®')),
       tbirthday DATE,
       ttitle VARCHAR2(10)
       );
CREATE TABLE teaching(
       tno NUMBER,
       cno VARCHAR(6),
       language VARCHAR2(10) CHECK(language IN('Chinese','Bilingual','English')),
       CONSTRAINT constraint_tno FOREIGN KEY (tno) REFERENCES teacher(tno),
       CONSTRAINT constraint_cno FOREIGN KEY (cno) REFERENCES course(cno)
       );
