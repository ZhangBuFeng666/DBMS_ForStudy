DELETE FROM student;
DELETE FROM class;
DELETE FROM teaching;
DELETE FROM sc;
DELETE FROM course;
DELETE FROM teacher;
INSERT ALL
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0801','软件0801','软件工程','软件开发','24')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0802','软件0802','软件工程','软件开发','26')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0803','软件0803','软件工程','数字媒体','25')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0804','软件0804','软件工程','软件开发','25')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0805','软件0805','软件工程','数字媒体','24')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0806','软件0806','软件工程','软件开发','24')
SELECT 1 FROM dual;
SELECT * FROM class;

INSERT ALL
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300010', '李在', '男', TO_DATE('1991-10-1', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300012', '葛畅', '男', TO_DATE('1990-8-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300015', '刘晶', '女', TO_DATE('1990-5-22', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300020', '杨敏', '女', TO_DATE('1989-1-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300030', '胡贤斌', '男', TO_DATE('1990-10-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300048', '赵鸿泽', '男', TO_DATE('1979-6-6', 'YYYY-MM-DD'), 'Rj0802')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300050', '王威', '男', TO_DATE('1990-6-10', 'YYYY-MM-DD'), 'Rj0802')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300067', '赵玮', '女', TO_DATE('1990-8-21', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300075', '王娜娜', '女', TO_DATE('1991-9-23', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300088', '秦键', '男', TO_DATE('1989-3-1', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300100', '田邦仪', '女', TO_DATE('1990-2-26', 'YYYY-MM-DD'), 'Rj0804')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300148', '赵心砚', '男', TO_DATE('1991-4-25', 'YYYY-MM-DD'), 'Rj0805')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300150', '杨青', '女', TO_DATE('1989-11-15', 'YYYY-MM-DD'), 'Rj0805')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300160', '杨玲玲', '女', TO_DATE('1990-12-12', 'YYYY-MM-DD'), 'Rj0806')
SELECT 1 FROM dual;

SELECT * FROM student;

INSERT ALL
INTO course(cno, cname, ccredit) VALUES ('800001', '计算机基础', 4)
INTO course(cno, cname, ccredit) VALUES ('800002', '程序设计语言', 4)
INTO course(cno, cname, ccredit) VALUES ('800003', '数据结构', 4)
INTO course(cno, cname, ccredit) VALUES ('810011', '数据库系统', 4)
INTO course(cno, cname, ccredit) VALUES ('810013', '计算机网络', 3)
INTO course(cno, cname, ccredit) VALUES ('810015', '微机原理与应用', 4)
SELECT 1 FROM dual;

SELECT * FROM course;

INSERT ALL
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000001', '李英', '女', TO_DATE('1975-11-3', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000002', '王大山', '男', TO_DATE('1969-3-2', 'YYYY-MM-DD'), '副教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000003', '张朋', '男', TO_DATE('1970-2-13', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000004', '陈为军', '男', TO_DATE('1985-8-14', 'YYYY-MM-DD'), '助教')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000005', '宋浩然', '男', TO_DATE('1976-4-23', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000006', '许红霞', '女', TO_DATE('1966-2-12', 'YYYY-MM-DD'), '副教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000007', '徐永军', '男', TO_DATE('1962-1-24', 'YYYY-MM-DD'), '教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000008', '李桂菁', '女', TO_DATE('1960-12-15', 'YYYY-MM-DD'), '教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000009', '王一凡', '女', TO_DATE('1974-12-8', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000010', '田峰', '男', TO_DATE('1988-1-18', 'YYYY-MM-DD'), '助教')
SELECT 1 FROM dual;

SELECT * FROM teacher;

INSERT ALL
INTO sc(sno, cno, grade) VALUES ('08300012', '800003', 58)
INTO sc(sno, cno, grade) VALUES ('08300015', '800003', NULL)
INTO sc(sno, cno, grade) VALUES ('08300020', '800001', NULL)
INTO sc(sno, cno, grade) VALUES ('08300020', '800003', 91)
INTO sc(sno, cno, grade) VALUES ('08300030', '800003', 27)
INTO sc(sno, cno, grade) VALUES ('08300048', '800003', 95)
INTO sc(sno, cno, grade) VALUES ('08300050', '800004', 60)
INTO sc(sno, cno, grade) VALUES ('08300050', '800001', 85)
INTO sc(sno, cno, grade) VALUES ('08300050', '800002', 67)
INTO sc(sno, cno, grade) VALUES ('08300067', '800004', 76)
INTO sc(sno, cno, grade) VALUES ('08300075', '800002', 10)
INTO sc(sno, cno, grade) VALUES ('08300100', '810011', 57)
INTO sc(sno, cno, grade) VALUES ('08300148', '810011', 58)
INTO sc(sno, cno, grade) VALUES ('08300150', '810011', 99)
INTO sc(sno, cno, grade) VALUES ('08300160', '810011', 71)
INTO sc(sno, cno, grade) VALUES ('08300160', '800002', 95)
INTO sc(sno, cno, grade) VALUES ('08300160', '800003', 70)
INTO sc(sno, cno, grade) VALUES ('08300160', '800004', 78)


SELECT 1 FROM dual;

SELECT * FROM sc;

INSERT ALL
INTO teaching(cno, tno, language) VALUES ('800001', '000001', 'English')
INTO teaching(cno, tno, language) VALUES ('800001', '000005', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000002', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000004', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000006', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800003', '000007', 'English')
INTO teaching(cno, tno, language) VALUES ('800003', '000002', 'Bilingual')
INTO teaching(cno, tno, language) VALUES ('810011', '000003', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('810011', '000007', 'English')
INTO teaching(cno, tno, language) VALUES ('810013', '000004', 'English')
INTO teaching(cno, tno, language) VALUES ('810013', '000008', 'Bilingual')
INTO teaching(cno, tno, language) VALUES ('810015', '000002', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('810015', '000004', 'English')
SELECT 1 FROM dual;

SELECT * FROM teaching;
COMMIT;

DELETE FROM student;
DELETE FROM class;
DELETE FROM teaching;
DELETE FROM sc;
DELETE FROM course;
DELETE FROM teacher;
INSERT ALL
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0801','软件0801','软件工程','软件开发','24')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0802','软件0802','软件工程','软件开发','26')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0803','软件0803','软件工程','数字媒体','25')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0804','软件0804','软件工程','软件开发','25')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0805','软件0805','软件工程','数字媒体','24')
INTO class(classno,classname,classmajor,classdept,studentnumber) VALUES ('Rj0806','软件0806','软件工程','软件开发','24')
SELECT 1 FROM dual;
SELECT * FROM class;

INSERT ALL
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300010', '李在', '男', TO_DATE('1991-10-1', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300012', '葛畅', '男', TO_DATE('1990-8-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300015', '刘晶', '女', TO_DATE('1990-5-22', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300020', '杨敏', '女', TO_DATE('1989-1-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300030', '胡贤斌', '男', TO_DATE('1990-10-8', 'YYYY-MM-DD'), 'Rj0801')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300048', '赵鸿泽', '男', TO_DATE('1979-6-6', 'YYYY-MM-DD'), 'Rj0802')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300050', '王威', '男', TO_DATE('1990-6-10', 'YYYY-MM-DD'), 'Rj0802')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300067', '赵玮', '女', TO_DATE('1990-8-21', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300075', '王娜娜', '女', TO_DATE('1991-9-23', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300088', '秦键', '男', TO_DATE('1989-3-1', 'YYYY-MM-DD'), 'Rj0803')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300100', '田邦仪', '女', TO_DATE('1990-2-26', 'YYYY-MM-DD'), 'Rj0804')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300148', '赵心砚', '男', TO_DATE('1991-4-25', 'YYYY-MM-DD'), 'Rj0805')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300150', '杨青', '女', TO_DATE('1989-11-15', 'YYYY-MM-DD'), 'Rj0805')
INTO student(sno, sname, ssex, sbirthday, classno) VALUES ('08300160', '杨玲玲', '女', TO_DATE('1990-12-12', 'YYYY-MM-DD'), 'Rj0806')
SELECT 1 FROM dual;

SELECT * FROM student;

INSERT ALL
INTO course(cno, cname, ccredit) VALUES ('800001', '计算机基础', 4)
INTO course(cno, cname, ccredit) VALUES ('800002', '程序设计语言', 4)
INTO course(cno, cname, ccredit) VALUES ('800003', '数据结构', 4)
INTO course(cno, cname, ccredit) VALUES ('810011', '数据库系统', 4)
INTO course(cno, cname, ccredit) VALUES ('810013', '计算机网络', 3)
INTO course(cno, cname, ccredit) VALUES ('810015', '微机原理与应用', 4)
SELECT 1 FROM dual;

SELECT * FROM course;

INSERT ALL
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000001', '李英', '女', TO_DATE('1975-11-3', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000002', '王大山', '男', TO_DATE('1969-3-2', 'YYYY-MM-DD'), '副教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000003', '张朋', '男', TO_DATE('1970-2-13', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000004', '陈为军', '男', TO_DATE('1985-8-14', 'YYYY-MM-DD'), '助教')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000005', '宋浩然', '男', TO_DATE('1976-4-23', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000006', '许红霞', '女', TO_DATE('1966-2-12', 'YYYY-MM-DD'), '副教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000007', '徐永军', '男', TO_DATE('1962-1-24', 'YYYY-MM-DD'), '教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000008', '李桂菁', '女', TO_DATE('1960-12-15', 'YYYY-MM-DD'), '教授')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000009', '王一凡', '女', TO_DATE('1974-12-8', 'YYYY-MM-DD'), '讲师')
INTO teacher(tno, tname, tsex, tbirthday, ttitle) VALUES ('000010', '田峰', '男', TO_DATE('1988-1-18', 'YYYY-MM-DD'), '助教')
SELECT 1 FROM dual;

SELECT * FROM teacher;

INSERT ALL
INTO sc(sno, cno, grade) VALUES ('08300012', '800003', 58)
INTO sc(sno, cno, grade) VALUES ('08300015', '800003', NULL)
INTO sc(sno, cno, grade) VALUES ('08300020', '800001', NULL)
INTO sc(sno, cno, grade) VALUES ('08300020', '800003', 91)
INTO sc(sno, cno, grade) VALUES ('08300030', '800003', 27)
INTO sc(sno, cno, grade) VALUES ('08300048', '800003', 95)
INTO sc(sno, cno, grade) VALUES ('08300050', '800004', 60)
INTO sc(sno, cno, grade) VALUES ('08300050', '800001', 85)
INTO sc(sno, cno, grade) VALUES ('08300050', '800002', 67)
INTO sc(sno, cno, grade) VALUES ('08300067', '800004', 76)
INTO sc(sno, cno, grade) VALUES ('08300075', '800002', 10)
INTO sc(sno, cno, grade) VALUES ('08300100', '810011', 57)
INTO sc(sno, cno, grade) VALUES ('08300148', '810011', 58)
INTO sc(sno, cno, grade) VALUES ('08300150', '810011', 99)
INTO sc(sno, cno, grade) VALUES ('08300160', '810011', 71)
INTO sc(sno, cno, grade) VALUES ('08300160', '800002', 95)
INTO sc(sno, cno, grade) VALUES ('08300160', '800003', 70)
INTO sc(sno, cno, grade) VALUES ('08300160', '800004', 78)
SELECT 1 FROM dual;

SELECT * FROM sc;
SELECT * FROM sc
JOIN student s ON s.sno=sc.sno
WHERE ssex='女';

INSERT ALL
INTO teaching(cno, tno, language) VALUES ('800001', '000001', 'English')
INTO teaching(cno, tno, language) VALUES ('800001', '000005', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000002', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000004', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800002', '000006', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('800003', '000007', 'English')
INTO teaching(cno, tno, language) VALUES ('800003', '000002', 'Bilingual')
INTO teaching(cno, tno, language) VALUES ('810011', '000003', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('810011', '000007', 'English')
INTO teaching(cno, tno, language) VALUES ('810013', '000004', 'English')
INTO teaching(cno, tno, language) VALUES ('810013', '000008', 'Bilingual')
INTO teaching(cno, tno, language) VALUES ('810015', '000002', 'Chinese')
INTO teaching(cno, tno, language) VALUES ('810015', '000004', 'English')
SELECT 1 FROM dual;

SELECT * FROM teaching;
COMMIT;

