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
import java.io.*;

public class MySQLJdbcPrepareMultiCharsetTest {
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
                    System.out.println(resultSetMetaData.getColumnName(i) + ":" + 
                                        resultSetMetaData.getColumnTypeName(i) + ":" + out);
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
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&characterEncoding=utf8";
        } else {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false&characterEncoding=utf8";
        }
    
        try (Connection conn = DriverManager.getConnection(url_jdbc, info);
             Statement statement = conn.createStatement()) {
            ResultSet resultSet;
            statement.executeUpdate("drop table if exists bin_mb_test");
            statement.executeUpdate("create table bin_mb_test (" +
                                        "c_int1 tinyint," +                 // 1
                                        "c_uint1 tinyint unsigned," +       // 2
                                        "c_int2 smallint," +                // 3
                                        "c_uint2 smallint unsigned," +      // 4
                                        "c_int3 mediumint," +               // 5
                                        "c_uint3 mediumint unsigned," +     // 6
                                        "c_int4 int," +                     // 7
                                        "c_uint4 int unsigned," +           // 8
                                        "c_int8 bigint," +                  // 9
                                        "c_uint8 bigint unsigned," +        // 10
                                        "c_f4 float4," +                    // 11
                                        "c_f8 double," +                    // 12
                                        "c_num numeric(10, 4)," +           // 13
                                        "c_char char," +                    // 14
                                        "c_char_n char(10)," +              // 15
                                        "c_varchar varchar(10)," +          // 16
                                        "c_text text," +                    // 17
                                        "c_bin binary(10)," +               // 18
                                        "c_varbin varbinary(10)," +         // 19
                                        "c_blob blob," +                    // 20
                                        "c_bit bit(55)," +                  // 21
                                        "c_year year," +                    // 22
                                        "c_date date," +                    // 23
                                        "c_time time," +                    // 24
                                        "c_time5 time(5)," +                // 25
                                        "c_datetime datetime," +            // 26
                                        "c_datetime5 datetime(5)," +        // 27
                                        "c_json json," +                    // 28
                                        "c_enum enum('中')" +               // 29
                                    ") charset=utf8");

            String insertSQL = "INSERT INTO bin_mb_test VALUES (" +
                                    "?," + // 1, int1
                                    "?," + // 2, uint1
                                    "?," + // 3, int2
                                    "?," + // 4, uint2
                                    "?," + // 5, int3
                                    "?," + // 6, uint3
                                    "?," + // 7, int4
                                    "?," + // 8, uint4
                                    "?," + // 9, int8
                                    "?," + // 10, uint8
                                    "?," + // 11, f4
                                    "?," + // 12, f8
                                    "?," + // 13, numeirc
                                    "?," + // 14, char
                                    "?," + // 15, char(10)
                                    "?," + // 16, varchar
                                    "?," + // 17, text
                                    "?," + // 18, binary
                                    "?," + // 19, varbinary
                                    "?," + // 20, blob
                                    "?," + // 21, bit(55)
                                    "?," + // 22, year
                                    "?," + // 23, date
                                    "?," + // 24, time
                                    "?," + // 25, time5
                                    "?," + // 26, datetime
                                    "?," + // 27, datetime5
                                    "?," + // 28, json
                                    "?" + // 29, enum
                                ")";

            try (PreparedStatement pi = conn.prepareStatement(insertSQL)) {
                pi.setInt(1, 127);  // int1
                pi.setInt(2, 128);  // uint1
                pi.setInt(3, 128);  // int2
                pi.setInt(4, 128);  // uint2
                pi.setInt(5, 128);  // int3
                pi.setInt(6, 128);  // uint3
                pi.setInt(7, 128);  // int4
                pi.setInt(8, 128);  // uint4
                pi.setInt(9, 128);  // int8
                pi.setInt(10, 128);  // uint8
                pi.setInt(11, 128);  // f4
                pi.setInt(12, 128);  // f8
                pi.setInt(13, 128);  // numeric
                pi.setString(14, "a"); // char
                pi.setString(15, "中"); // char(10)
                pi.setString(16, "中"); // varchar
                pi.setString(17, "中"); // text
                pi.setString(18, "中"); // binary
                pi.setString(19, "中"); // varbinary
                pi.setString(20, "中"); // blob

                /*
                String val = "中";
                byte[] bytes = val.getBytes(java.nio.charset.StandardCharsets.UTF_8);
                ByteArrayInputStream inputStream = new ByteArrayInputStream(bytes);
                pi.setBlob(21, inputStream, bytes.length); // bit(55)
                */
                pi.setString(21, "中"); // date

                pi.setInt(22, 2022);  // year
                pi.setString(23, "2022-10-1"); // date
                pi.setString(24, "10:10:10"); // time
                pi.setString(25, "10:10:10.55555"); // time5
                pi.setString(26, "2022-10-1 10:10:10"); // datetime
                pi.setString(27, "2022-10-1 10:10:10.55555"); // datetime5

                pi.setString(28, "{}"); // json
                pi.setString(29, "中"); // enum

                pi.executeUpdate();
                System.out.println("Insert success");
            }
                
            PreparedStatement ps = conn.prepareStatement("SELECT * FROM bin_mb_test");
            resultSet = ps.executeQuery();
            print_res(resultSet);

            ps.clearParameters();
            ps.close();
            resultSet.close();
        }
    }
}
