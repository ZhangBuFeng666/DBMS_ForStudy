/*
3、  简单的数据操作
（1） 查询所有同学的所有基本信息。
（2） 查询所有男同学的学号、姓名、出生日期
（3） 在基本表Student中增加addr：varchar(20)列，然后将其长度由20改为25
（4） 在基本表student中增加register_date：date列，并为其设置默认值为当前系统时间，再删除该列
（5） 在基本表student中为age列，增加默认值为18
（6） 在基本表sc中将sname设置为唯一值（unique）
（7） 基本表course中创建索引：（sno，cno desc）
（8） 在基本表Student中增加约束条件：男生年龄小于23岁，女生年龄小于21岁。
（9） 创建视图View_80，存放成绩高于80分的选课信息, 显示学号、课程号和成绩，使用with check option选项。
（10）  在视图View_80中查询成绩高于90的选课信息。
（11）  在视图View_80中依次插入如下元组：
08301168，810011，87（可插入成功）
08301167，810011，78（插入不成功）
（12）  在视图View_80中依次修改如下元组
将（08301168，810011）所对应的成绩改为90；（可修改成功）
将（08301168，810011）所对应的成绩改为70；（修改不成功）
（13）  在视图View_80中删除如下元组：sno=08301168，cno=810011
（14）  查询所有在“1980-01-01”之前出生的女同学的学号、姓名、性别、出生日期
（15）  查询所有姓“李”的男同学的学号、姓名、性别、出生日期
（16）  查询所有用英文授课的教师的教师号、姓名及英语授课的门数
（17）  查询所有职称不是“讲师”的教师的教师号、姓名、职称
（18）  查询虽然选修了课程，但未参加考试的所有同学的学号
（19）  查询所有考试不及格的同学的学号、成绩，并按成绩降序排列
（20）  查询在1970年出生的教师的教师号、姓名、出生日期
（21）  查询各个课程号的选课人数
（22）  查询讲授2门课以上的教师号
（23）  查询选修了800001课程的学生平均分数、最低分数和最高分数
（24）  查询1960年以后出生的，职称为讲师的教师的姓名、出生日期，并按出生日期升序排列。
4、  复杂数据查询
（1） 创建视图new_View显示所有同学的选课及成绩情况，列出学生的学号、姓名、班号、课程名称和成绩。
（2） 在视图new_View中查询“软件0801”班的同学的选课及成绩情况，显示学号、姓名、课程名称、成绩
（3） 在视图new_View中插入如下元组：08300010，李在，R,j0801，数据库系统，88（此为不成功的操作）
（4） 查询所有同学的学分情况（假设课程成绩>=60时可获得该门课程的学分），显示学号、姓名、总学分（用JOIN）
（5） 查询所有同学的平均成绩及选课门数，显示学号、姓名、平均成绩、选课门数
（6） 查询所有选修了课程但未参加考试的所有同学及相应的课程，显示学号、姓名课程号、课程名称
（7） 查询所有选修了课程但考试不及格的所有同学及相应的课程，显示学号、姓名、课程号、课程名称、成绩
（8） 查询选修了课程名为“程序设计语言”的所有同学及成绩情况，显示学生姓名、课程成绩（用ANY运算符）
（9） 查询“软件开发系”的所有同学及成绩情况，显示学号、姓名、班级名称、课程号、课程名称、成绩
（10）  查询所有教师的任课情况，显示教师姓名、课程名称
（11）  查询成绩低于同门课程平均成绩的信息，显示学生学号、姓名、课程名称及低于平均成绩的值（即比平均成绩低多少）
（12）  查询和“葛畅”在同一班级的同学的姓名（使用子查询）
（13）  查询没有选修“计算机基础”课程的学生姓名（用NOT EXISTS）
（14）  查询主讲“数据库系统”和主讲“数据结构”的教师姓名（用UNION）
（15）  查询讲授了所有课程的教师的姓名
（16）  查询同时选修学课程800001和800002的女同学的姓名
（17）  查询既没选修课程800001，又没选修800002的女同学的姓名
（18）  查询有一门课程成绩为95分的女同学的姓名
（19）  查询选课数量大于3门的女同学的姓名
（20）  查询平均成绩大于80分的男同学的姓名
（21）  查询徐永军老师所教的每一门课程的平均成绩
（22）  查询男同学每一个年龄组的人数,要求按人数升序输出人数超过20人的年龄组
（23）  查询每门课程成绩都大于90分的学生姓名
（24）  查询比所有女同学年龄要大的男同学的姓名
（25）  查询未选修800002课程的女同学的姓名
（26）  查询所有课程成绩都及格的学生姓名
（27）  查询选修课所有课程的学生姓名
（28）  查询选修了葛畅同学所选修的所有课程的学生姓名
（29）  查询平均成绩最高的学生姓名
（30）  找出比所在班级平均成绩高的学生信息
5．用数据操纵语言DML完成下列对3张表Student、Course、SC的各种更新操作：
（1） 将选修徐永军老师所教课程的女同学的成绩提高5%
（2） 在基本表Student中检索每一门课程成绩都大于等于80分的学生学号、姓名、性别，并把检索到的值送往另一个已存在的基本表STUD（S#，SNAME，SEX）。
（3） 在基本表SC中删除尚无成绩的选课记录。
（4） 把王威同学的学习选课和成绩全部删除。
（5） 把选修数据结构课不及格的成绩全改为空值。
（6） 把低于总平均成绩的女同学的成绩提高5%
（7） 在基本表SC中修改800004课程的成绩，若成绩小于等于75分时提高5%，若成绩大于75分时提高4%（用两个UPDATE语句实现）。

*/
--3.1
SELECT * FROM student;
--3.2
SELECT sno,sname,sbirthday FROM student WHERE ssex = '男';
--3.3
ALTER TABLE student ADD addr VARCHAR(20);
ALTER TABLE student MODIFY addr VARCHAR(25);
--3.4
ALTER TABLE student ADD register_date DATE DEFAULT SYSDATE;
ALTER TABLE student DROP COLUMN register_date;
--3.5
ALTER TABLE student ADD age INTEGER DEFAULT 18;
--3.6
ALTER TABLE student ADD CONSTRAINT unique_sname UNIQUE (sname);
--3.7(sc/course)
DROP INDEX sc_sno_cno_desc;
CREATE INDEX sc_sno_cno_desc ON sc(sno,cno ASC);
--3.8
CREATE TABLE studentNew(
       sno VARCHAR2(8) PRIMARY KEY,
       sname VARCHAR2(8) NOT NULL,
       ssex VARCHAR2(2) CHECK(ssex IN ('男','女')),
       sbirthday DATE,
       classno VARCHAR2(6),
       Totalcredit SMALLINT DEFAULT 0,
       age INTEGER DEFAULT 18,
       CONSTRAINT constrint_classno_new FOREIGN KEY (classno) REFERENCES class(classno),
       --新约束条件
       CONSTRAINT boy_y_23_and_girl_y_21 CHECK(
                  (ssex = '男' AND age<23) OR
                  (ssex = '女' AND age<21)
       )                  
);
INSERT INTO studentNew(sno,sname,ssex,sbirthday,classno,Totalcredit,age)
SELECT sno,sname,ssex,sbirthday,classno,Totalcredit,age FROM student;
DROP TABLE student;
ALTER TABLE studentNew RENAME TO student;
--3.9
CREATE VIEW View_80 AS
SELECT sno,cno,grade
FROM sc
WHERE grade>80
WITH CHECK OPTION;
--3.10
SELECT * FROM View_80 WHERE grade>90;
--3.11
INSERT INTO View_80(sno,cno,grade) VALUES ('08301168','810011','87');
INSERT INTO View_80(sno,cno,grade) VALUES ('08301167','810011','78');
--3.12
UPDATE View_80 SET grade=90 WHERE sno='08301168' AND cno='810011'; 
UPDATE View_80 SET grade=70 WHERE sno='08301168' AND cno='810011'; 
--3.13
DELETE FROM View_80 WHERE sno='08301168' AND cno='810011';
--3.14
SELECT sno,sname,ssex,sbirthday
FROM student
WHERE ssex='女' and sbirthday<TO_DATE('1980-1-1','YYYY-MM-DD');
--3.15
SELECT sno,sname,ssex,sbirthday FROM student WHERE sname LIKE '李%';
--3.16
SELECT LPAD(t.tno,6,'0') AS tno,t.tname,COUNT(te.cno) AS eng_course_count
FROM teaching te
JOIN teacher t ON te.tno=t.tno
WHERE te.language = 'English'
GROUP BY t.tno, t.tname;
--3.17
SELECT tno,tname,ttitle 
FROM teacher
WHERE ttitle <> '讲师'
--3.18
SELECT DISTINCT sno
FROM sc
WHERE grade IS NULL;
--3.19
SELECT sno,grade
FROM sc
WHERE grade < 60
ORDER BY grade DESC;
--3.20
SELECT tno,tname,tbirthday FROM teacher WHERE tbirthday < TO_DATE('1970-1-1','YYYY-MM-DD');
--3.21
SELECT cno,COUNT(DISTINCT sno)
FROM sc
GROUP BY cno;
--3.22
SELECT tno
FROM teaching
GROUP BY tno
HAVING COUNT(cno)>2;
--3.23
SELECT 
       AVG(grade),
       MIN(grade),
       MAX(grade)
FROM sc
WHERE cno = '810011';
--3.24
SELECT tname,tbirthday
FROM teacher
WHERE tbirthday>=TO_DATE('1960-1-1','YYYY-MM-DD') AND ttitle='讲师'
ORDER BY tbirthday ASC;
--4.1
DROP VIEW new_View;
CREATE VIEW new_View AS
SELECT st.sno,st.sname,st.classno,course.cname,sc.grade
FROM student st
JOIN sc ON sc.sno=st.sno
JOIN course ON course.cno=sc.cno;
SELECT * FROM new_View;
--4.2
SELECT sno,sname,cname,grade
FROM new_View
WHERE classno='Rj0801';
--4.3
--尝试通过--INSTAND OF触发器--处理
DROP TRIGGER trg_insert_new_view;
CREATE OR REPLACE TRIGGER trg_insert_new_view
INSTEAD OF INSERT ON new_View
FOR EACH ROW
DECLARE
    v_cno course.cno%TYPE;
    v_course_found BOOLEAN := TRUE;
BEGIN
    -- 检查课程名称对应的课程号
    BEGIN
        SELECT cno INTO v_cno
        FROM course
        WHERE cname = :NEW.cname;
    EXCEPTION
        WHEN NO_DATA_FOUND THEN
            DBMS_OUTPUT.PUT_LINE('未找到课程: ' || :NEW.cname);
            v_course_found := FALSE;
        WHEN TOO_MANY_ROWS THEN
            DBMS_OUTPUT.PUT_LINE('找到多个匹配课程: ' || :NEW.cname);
            v_course_found := FALSE;
    END;
    
    IF NOT v_course_found THEN
        RETURN;
    END IF;
    
    -- 插入学生信息
    BEGIN
        INSERT INTO student (sno, sname, classno, Totalcredit)
        VALUES (:NEW.sno, :NEW.sname, :NEW.classno, 0);
    EXCEPTION
        WHEN DUP_VAL_ON_INDEX THEN
            -- 学生记录已存在，不做任何处理
            NULL;
    END;
    
    -- 插入选课信息
    BEGIN
        INSERT INTO sc (sno, cno, grade)
        VALUES (:NEW.sno, v_cno, 
                CASE 
                    WHEN :NEW.grade IS NULL THEN NULL
                    ELSE TO_NUMBER(:NEW.grade)
                END);
    EXCEPTION
        WHEN VALUE_ERROR THEN
            DBMS_OUTPUT.PUT_LINE('成绩格式错误: ' || :NEW.grade);
        WHEN DUP_VAL_ON_INDEX THEN
            DBMS_OUTPUT.PUT_LINE('该学生已选此课程: ' || :NEW.sno || ', ' || v_cno);
    END;
EXCEPTION
    WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('发生错误: ' || SQLERRM);
        RAISE;
END;
/


INSERT INTO new_View(sno,sname,classno,cname,grade)
       VALUES ('0830001','李在','Rj0801','数据库系统','88');
--4.4
SELECT s.sno,s.sname,SUM(CASE WHEN sc.grade>=60
       THEN course.ccredit ELSE 0 END)
FROM student s
LEFT JOIN
     sc ON sc.sno=s.sno
LEFT JOIN
     course ON course.cno=sc.cno
GROUP BY s.sno,s.sname
ORDER BY s.sno;
--4.5
SELECT s.sno,s.sname,AVG(sc.grade),COUNT(sc.cno)
FROM student s
JOIN sc ON sc.sno=s.sno
GROUP BY s.sno,s.sname;
--4.6
SELECT s.sno,s.sname,c.cno,c.cname
FROM student s
JOIN sc ON s.sno = sc.sno
JOIN course c ON sc.cno=c.cno
WHERE sc.grade IS NULL;
--4.7
SELECT s.sno,s.sname,c.cno,c.cname,sc.grade
FROM student s
JOIN sc ON sc.sno = s.sno
JOIN course c ON c.cno=sc.cno
WHERE sc.grade < 60;
--4.8
SELECT s.sname,sc.grade
FROM student s
JOIN sc ON s.sno = sc.sno
WHERE sc.cno = ANY(
      SELECT cno
      FROM course
      WHERE cname = '数据结构');
--4.9
SELECT s.sno,s.sname,class.classname,c.cno,c.cname,sc.grade
FROM student s
JOIN class ON s.classno = class.classno
JOIN sc ON sc.sno=s.sno
JOIN course c ON c.cno= sc.cno
WHERE classdept = '软件开发';
--4.10
SELECT t.tname,c.cname
FROM teacher t
JOIN teaching ON teaching.tno = t.tno
JOIN course c ON c.cno = teaching.cno;
--4.11
SELECT s.sno,sname,cname,AVG_GRADE-grade
FROM student s
JOIN sc ON sc.sno=s.sno
JOIN course c ON sc.cno=c.cno
JOIN(SELECT cno,AVG(grade) AS AVG_GRADE
     FROM sc
     GROUP BY cno)avg_grades ON avg_grades.cno=sc.cno
WHERE sc.grade<avg_grades.AVG_GRADE;
--4.12
SELECT sname
FROM student
WHERE classno=(
SELECT classno 
FROM student
WHERE sname = '葛畅' ) AND sname !='葛畅';
--4.13
SELECT s.sname
FROM student s
WHERE NOT EXISTS(
      SELECT 1
      FROM sc
      JOIN course c ON sc.cno = c.cno
      WHERE sc.sno = s.sno AND c.cname = '计算机基础');
--4.14
SELECT t.tname
FROM teacher t
JOIN teaching ON teaching.tno = t.tno
JOIN course c ON c.cno = teaching.cno
WHERE c.cname='数据库系统'
UNION
SELECT t.tname
FROM teacher t
JOIN teaching ON teaching.tno = t.tno
JOIN course c ON c.cno = teaching.cno
WHERE c.cname='数据结构';
--4.15
SELECT teacher.tname
FROM teacher
WHERE(
     SELECT COUNT(DISTINCT course.cno)
     FROM course
     )=(
     SELECT COUNT(DISTINCT teaching.cno)
     FROM teaching
     WHERE teaching.tno = teacher.tno);
--4.16
--一种
SELECT s.sname
FROM student s
WHERE s.ssex='女' AND
      EXISTS(
      SELECT 1
      FROM sc
      WHERE sc.sno=s.sno
      AND  sc.cno = '800002'
      ) AND
      EXISTS(
      SELECT 1
      FROM sc
      WHERE sc.sno=s.sno
      AND  sc.cno = '800001'
      );
--二种
SELECT s.sname
FROM student s
JOIN sc sc1 ON s.sno = sc1.sno
JOIN sc sc2 ON s.sno = sc2.sno
WHERE s.ssex = '女'
      AND sc1.cno='800001'
      AND sc2.cno='800002';
--三种
SELECT s.sname
FROM student s
JOIN sc ON s.sno = sc.sno
WHERE s.ssex = '女'
      AND sc.cno IN('800001','800002') 
GROUP BY s.sno,s.sname
HAVING COUNT(DISTINCT sc.cno)=2;  
--4.17
--一种
SELECT sname
FROM student s
WHERE ssex = '女'
      AND NOT EXISTS (SELECT 1 FROM sc WHERE s.sno = sc.sno AND cno = '800001')
      AND NOT EXISTS (SELECT 1 FROM sc WHERE s.sno = sc.sno AND cno = '800002');
--二种
SELECT s.sname
FROM student s
WHERE ssex='女'
      AND NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE sc.sno=s.sno AND cno IN ('800001','800002'));
--三种
SELECT s.sname
FROM student s
WHERE ssex='女'
      AND s.sno NOT IN(
      SELECT DISTINCT sno
      FROM sc
      WHERE cno IN ('800001','800002')
);
--4.18
SELECT sname
FROM student
NATURAL JOIN sc
WHERE ssex='女' AND
      grade = 95;
--4.19
--(1)
SELECT DISTINCT sname
FROM student
WHERE ssex='女' AND EXISTS(
      SELECT 1
      FROM sc
      WHERE student.sno = sc.sno
      GROUP BY sno
      HAVING COUNT(cno)>2);
--(2)
SELECT sname
FROM student s
NATURAL JOIN sc
WHERE ssex='女'
GROUP BY sname HAVING COUNT(cno)>2;
--4.20
SELECT sname
FROM student s
JOIN sc ON sc.sno=s.sno
WHERE ssex='男'
GROUP BY sname HAVING AVG(sc.grade)>80;
--4.21
SELECT c.cname,AVG(sc.grade) AS avgs
FROM sc
JOIN course c ON c.cno = sc.cno
JOIN teaching ti ON ti.cno = sc.cno
JOIN teacher t ON t.tno = ti.tno
WHERE t.tname = '徐永军'
GROUP BY c.cname;
--4.22
SELECT age,COUNT(*) AS num
FROM (
     SELECT EXTRACT(YEAR FROM SYSDATE) - EXTRACT(YEAR FROM sbirthday) AS age
     FROM student
     WHERE ssex='男'
     )
GROUP BY age HAVING COUNT(*)>2
ORDER BY num ASC;
--4.23
--(1)
SELECT sname
FROM student s
JOIN sc ON sc.sno = s.sno
WHERE NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE sc.sno=s.sno AND (grade IS NULL OR grade<=90));
--(2)
SELECT sname
FROM student s
NATURAL JOIN sc
GROUP BY sno,sname HAVING MIN(grade)>90;
--4.24
SELECT sname
FROM student
WHERE ssex='男'
   AND  EXTRACT(YEAR FROM SYSDATE)-EXTRACT(YEAR FROM sbirthday)
      >ALL(
      SELECT EXTRACT(YEAR FROM SYSDATE) - EXTRACT(YEAR FROM Sbirthday)
      FROM student
      WHERE ssex='女');
--4.25
SELECT sname
FROM student s
WHERE ssex='女' AND NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE sc.sno=s.sno AND cno = '800003');
--4.26
SELECT sname
FROM student s
WHERE NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE s.sno =sc.sno AND grade<60);
--4.27
--一种理解：查询选修课所有的学生姓名
SELECT DISTINCT sname
FROM sc
LEFT JOIN student s ON sc.sno=s.sno;
--二种理解：查询选修了所有课程的学生姓名
--(1)
SELECT s.sname
FROM student s
JOIN sc ON s.sno = sc.sno
GROUP BY s.sno, s.sname
HAVING COUNT(DISTINCT sc.cno) = (SELECT COUNT(*) FROM course);
--(2)
SELECT sname
FROM student s
WHERE NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE NOT EXISTS(
            SELECT cno
            FROM course c
            WHERE s.sno=sc.sno AND sc.cno=c.cno)
      );

--4.28
SELECT s1.sname
FROM student s1
WHERE NOT EXISTS(
      SELECT 1
      FROM sc sc1
      JOIN student s2 ON s2.sno=sc1.sno
      WHERE s2.sname='葛畅' AND NOT EXISTS(
            SELECT 1
            FROM sc sc2
            WHERE sc2.sno=s1.sno AND sc2.cno=sc1.cno)
      )
      AND s1.sname<>'葛畅';
--4.29
SELECT sname
FROM student
NATURAL JOIN sc
GROUP BY sname
ORDER BY AVG(grade)
FETCH FIRST 1 ROW ONLY;
--4.30
SELECT sname
FROM student s
JOIN(
     SELECT sno,AVG(grade) AS avgg
     FROM student s1
     NATURAL JOIN sc sc1
     GROUP BY sno) stu_avg ON s.sno=stu_avg.sno
JOIN(
     SELECT classno,AVG(grade) AS avgg
     FROM student s2
     NATURAL JOIN sc sc2
     GROUP BY classno) clas_avg ON s.classno=clas_avg.classno
WHERE stu_avg.avgg>clas_avg.avgg;

--5.1
UPDATE sc
SET grade = grade*1.05
WHERE sno IN(
      SELECT s.sno
      FROM student s
      JOIN sc ON s.sno=sc.sno
      JOIN teaching ti ON ti.cno=sc.cno
      JOIN teacher t ON ti.tno=t.tno
      WHERE s.ssex='女' AND t.tname='徐永军'
);
--5.2
DROP TABLE STUD;
CREATE TABLE STUD(
       S# VARCHAR(8) PRIMARY KEY,
       SNAME VARCHAR2(8) NOT NULL,
       SEX VARCHAR2(2) CHECK(SEX IN ('男','女'))
       );
INSERT INTO STUD(S#,SNAME,SEX)
SELECT sno,sname,ssex
FROM student s
WHERE NOT EXISTS(
      SELECT 1
      FROM sc
      WHERE NOT EXISTS(SELECT 1 FROM SC WHERE sc.sno=s.sno)
      OR(sc.sno=s.sno AND sc.grade<80));

SELECT *
FROM STUD;
--5.3
DELETE FROM sc
WHERE grade IS NULL;
--5.4
DELETE FROM sc
WHERE sno=(SELECT sno FROM student WHERE sname='王威');
--5.5
UPDATE sc
SET grade = NULL
WHERE grade<60 AND cno=(SELECT cno FROM course WHERE cname='数据结构');
--5.6
UPDATE sc
SET grade=1.05*grade
WHERE sno IN (SELECT sno FROM student WHERE ssex='女') 
      AND grade<(SELECT AVG(grade) FROM sc);
--5.7
UPDATE sc
SET grade = LEAST(grade * 1.05,100)
WHERE cno = '810011'
      AND grade <= 75;
UPDATE sc
SET grade = LEAST(grade * 1.04,100)
WHERE cno = '810011'
      AND grade > 75;
  
--6.1
DROP PROCEDURE insert_student;
CREATE OR REPLACE PROCEDURE insert_student(
       stu_id IN VARCHAR2,
       sname IN VARCHAR2,
       gender IN VARCHAR2,
       birth IN DATE,
       cla_id IN VARCHAR2
)IS
BEGIN 
  INSERT INTO student(sno,sname,ssex,sbirthday,classno)
  VALUES(stu_id,sname,gender,birth,cla_id);
  
  
  DBMS_OUTPUT.put_line('学生记录插入成功');
EXCEPTION
     WHEN OTHERS THEN
       ROLLBACK;
       DBMS_OUTPUT.put_line('插入学生记录时出错？！：'||SQLERRM);
END insert_student;
/

CALL insert_student('08301099','阿布','男',TO_DATE('2000-01-30','YYYY-MM-DD'),'Rj0801');
--6.2
--6.3
--6.4
--6.5
--6.6

ROLLBACK;


























