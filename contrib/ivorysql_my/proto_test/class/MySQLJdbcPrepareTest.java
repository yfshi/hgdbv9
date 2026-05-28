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
import java.sql.PreparedStatement;
import java.util.Properties;
import java.sql.Time;
import java.sql.Timestamp;
import java.math.BigDecimal;
import java.sql.Blob;
import java.sql.SQLException;

public class MySQLJdbcPrepareTest {
    private static String host;
    private static String port = "3307";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String url_jdbc;

    public static void print_res(ResultSet resultSet) {
        try {
            ResultSetMetaData resultSetMetaData = resultSet.getMetaData();
            while (resultSet.next()) {
                for (int i = 1; i <= resultSetMetaData.getColumnCount(); i++) {
                    byte[] val = resultSet.getBytes(i);
                    String out = val != null ? new String(val) : null;
                    System.out.println(resultSetMetaData.getColumnName(i) + ":" +resultSetMetaData.getColumnTypeName(i) + ":" + out);
                }
            }
        } catch (SQLException e) {
            System.out.println("print_res failed:" + e.getMessage());
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length >= 5) {
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
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
        } else {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
        }
    
        try (Connection connection = DriverManager.getConnection(url_jdbc, info);
             Statement statement = connection.createStatement()) {
            ResultSet resultSet;
            ResultSetMetaData resultSetMetaData;
            statement.executeUpdate("drop table if exists t3");
            statement.executeUpdate("create table t3 (" +
                "c1 integer," +
                "c2 integer unsigned," +
                "c3 tinyint," +
                "c4 tinyint unsigned," +
                "c5 smallint," +
                "c6 smallint unsigned," +
                "c7 bigint," +
                "c8 bigint unsigned," +
                "c9 float4," +
                "c10 double," +
                "c11 bit(10)," +
                "c12 bool," +
                "c13 char(10)," +
                "c15 varchar(10)," +
                "c16 nvarchar(10)," +
                "c18 year," +
                "c19 date ," +
                "c20 time, " +
                "c21 timestamp(3)," +
                "c22 datetime(3)," +
                "c23 tinyblob," +
                "c24 blob," +
                "c25 mediumblob," +
                "c26 longblob," +
                "c27 point," +
                "c29 text," +
                "c32 enum('a', 'b')," +
                "c33 enum('a', 'b')," +
                "c34 char," +
                "c35 name" +
                ")");
            
            PreparedStatement p2 = connection.prepareStatement("select * from t3 where c1=?");
            p2.setObject(1, 1);
            resultSet = p2.executeQuery();
            print_res(resultSet);

            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("show tables like ?");
            p2.setString(1, "t%");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("show VARIABLES like ?");
            p2.setString(1, "enable_ivorysql%");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("show databases like ?");
            p2.setString(1, "public%");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("show COLUMNS FROM t3 like ?");
            p2.setString(1, "c1%");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("select oid,* from pg_proc where proname = ?");
            p2.setString(1, "backtrace");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            p2 = connection.prepareStatement("select oid,* from pg_class where relname = ?");
            p2.setString(1, "replication_slots");
            resultSet = p2.executeQuery();
            print_res(resultSet);
            p2.clearParameters();
            p2.close();
            resultSet.close();

            statement.execute("set sql_mode = 'STRICT_TRANS_TABLES,ansi_quotes'");
            statement.executeUpdate("insert into t3(c3) values(0)");
            PreparedStatement p3 = connection.prepareStatement("select * from t3 where c3=?");
            p3.setString(1, "abc");
            resultSet = p3.executeQuery();
            resultSetMetaData = resultSet.getMetaData();
            while (resultSet.next()) {
                for (int i = 1; i <= resultSetMetaData.getColumnCount(); i++) {
                    byte[] val = resultSet.getBytes(i);
                    String out = val != null ? new String(val) : null;
                    System.out.println(resultSetMetaData.getColumnName(i) + ":" +resultSetMetaData.getColumnTypeName(i) + ":" + out);
                }
            }
            p3.close();
            resultSet.close();

            try {
                p3 = connection.prepareStatement("update t3 set c1 = 1 where c3=?");
                p3.setString(1, "abc");
                p3.executeUpdate();
                p3.close();
            } catch (SQLException e) {
                // expect failed
                System.out.println("update failed:" + e.getMessage());
                p3.close();
            }

            try {
                p3 = connection.prepareStatement("delete from t3 where c3=?");
                p3.setString(1, "abc");
                p3.executeUpdate();
                p3.close();
            } catch (SQLException e) {
                // expect failed
                System.out.println("delete failed:" + e.getMessage());
                p3.close();
            }

            statement.execute("set sql_mode = 'ansi_quotes'");
            p3 = connection.prepareStatement("delete from t3 where c3=?");
            p3.setString(1, "abc");
            p3.executeUpdate();
            p3.close();
        }
    }
}
