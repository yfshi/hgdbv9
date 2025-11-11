/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.Statement;
import java.util.Properties;

public class MySQLJdbcTest {
    private static String host;
    private static String port = "3306";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String url_jdbc;

    public static void main(String[] args) throws Exception {
        if (args.length >=5) {
            host = args[0];
            port = args[1];
            dbname = args[2];
            user = args[3];
            password = args[4];
            if (args.length == 6) {
                jar_version = args[5];
            }
        }
        
        Properties info = new Properties();
        info.setProperty("HOST", host);
        info.setProperty("PORT", port);
        info.setProperty("DBNAME", dbname);
        info.setProperty("user", user);
        info.setProperty("password", password);

        if (jar_version.equals("new")) {
            url_jdbc = "jdbc:mysql://";
        } else {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useSSL=false";
        }

        try (Connection connection = DriverManager.getConnection(url_jdbc, info);
             Statement statement = connection.createStatement()) {
            ResultSet resultSet;
            ResultSetMetaData resultSetMetaData;
            statement.executeUpdate("drop table if exists t3");
            statement.executeUpdate("create table t3 (" +
                    "c1 integer," +
                    "c2 integer," +
                    "c3 integer," +
                    "c4 integer," +
                    "c5 smallint," +
                    "c6 smallint," +
                    "c7 bigint," +
                    "c8 bigint," +
                    "c9 float4," +
                    "c10 float8," +
                    "c11 bit(5)," +
                    "c12 bool," +
                    "c13 char(5)," +
                    "c15 varchar(5)" + 
                    ")");
            resultSet = statement.executeQuery("select * from t3");
            resultSetMetaData = resultSet.getMetaData();
            for (int i = 1; i <= resultSetMetaData.getColumnCount(); i++) {
                System.out.println(resultSetMetaData.getColumnName(i) + ":" + resultSetMetaData.getColumnTypeName(i));
            }

            resultSet = statement.executeQuery("explain (costs off) select * from t3 a join t3 b on a.c1=b.c2");
            resultSetMetaData = resultSet.getMetaData();
            for (int i = 1; i <= resultSetMetaData.getColumnCount(); i++) {
                System.out.println(resultSetMetaData.getColumnName(i) + ":" + resultSetMetaData.getColumnTypeName(i));
            }

            while (resultSet.next()) {
                final String res_getString = resultSet.getString(1);
                System.out.println(res_getString);
            }

            // test for jdbc.getBoolean and getInt
            statement.executeUpdate("drop table if exists t3");
            statement.executeUpdate("create table t3 (id int, c boolean)");
            statement.executeUpdate("insert into t3 values(1, true), (2, false)");
            resultSet = statement.executeQuery("select * from t3");
            while (resultSet.next()) {
                final boolean res_getBoolean = resultSet.getBoolean("c");
                final int res_getInt = resultSet.getInt("c");
                final String res_getString = resultSet.getString("c");
                System.out.println("res_getBoolean: " + res_getBoolean);
                System.out.println("res_getInt: " + res_getInt);
                System.out.println("res_getString: " + res_getString);
            }

            statement.executeUpdate("drop table if exists t3");
            statement.executeUpdate("create table t3 (id int, name varchar(50))");
            statement.executeUpdate("insert into t3(id, name) values('1', 'mysql')");
            statement.executeUpdate("insert into t3 values('2', 'ivorysql'), ('3', 'postgres')");
            statement.executeUpdate("update t3 set name='ivorysql-server' where id ='2'");
            resultSet = statement.executeQuery("select * from t3 order by id");
            resultSetMetaData = resultSet.getMetaData();
            while (resultSet.next()) {
                for (int i = 1; i <= resultSetMetaData.getColumnCount(); i++) {
                    System.out.println(resultSet.getObject(i));
                }
            }
        }
    }
}
