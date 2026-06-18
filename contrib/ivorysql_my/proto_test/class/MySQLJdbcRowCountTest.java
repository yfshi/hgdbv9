import java.sql.*;
import java.util.Properties;

public class MySQLJdbcRowCountTest
{
    public static void main(String[] args)
        throws Exception
    {
        if (args.length != 5)
        {
            System.out.println("Usage:");
            System.out.println("java MySQLJdbcRowCountTest <host> <port> <dbname> <user> <password>");
            return;
        }

        String url = "jdbc:mysql:///?allowPublicKeyRetrieval=true&useSSL=false";

        Properties p = new Properties();
        p.setProperty("HOST", args[0]);
        p.setProperty("PORT", args[1]);
        p.setProperty("DBNAME", args[2]);
        p.setProperty("user", args[3]);
        p.setProperty("password", args[4]);

        try (
            Connection conn = DriverManager.getConnection(url, p);
            Statement stmt = conn.createStatement())
        {
            /*
             * Case 1: CREATE TABLE
             * Expected: ROW_COUNT() = 0
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            executeAndCheckRowCount(stmt,
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)",
                0, true);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 2: ALTER TABLE
             * Expected: ROW_COUNT() = 0
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            executeAndCheckRowCount(stmt,
                "ALTER TABLE rowcount_test ADD COLUMN name VARCHAR(20)",
                0, true);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 3: DROP TABLE
             * Expected: ROW_COUNT() = 0
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            executeAndCheckRowCount(stmt,
                "DROP TABLE rowcount_test",
                0, true);


            /*
             * Case 4: INSERT
             * Expected: ROW_COUNT() = inserted rows, 3
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            executeAndCheckRowCount(stmt,
                "INSERT INTO rowcount_test VALUES (1),(2),(3)",
                3, true);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 5: UPDATE changed rows
             * Expected: ROW_COUNT() = changed rows, 3
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, val INT)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1,10),(2,20),(3,30)");
            executeAndCheckRowCount(stmt,
                "UPDATE rowcount_test SET val = 100 WHERE id <= 3",
                3, true);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 6: UPDATE no changed rows
             * Expected: ROW_COUNT() = 0
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, val INT)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1,100),(2,100),(3,100)");
            executeAndCheckRowCount(stmt,
                "UPDATE rowcount_test SET val = 100 WHERE id <= 3",
                0, false);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 7: DELETE
             * Expected: ROW_COUNT() = deleted rows, 3
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1),(2),(3),(4),(5)");
            executeAndCheckRowCount(stmt,
                "DELETE FROM rowcount_test WHERE id <= 3",
                3, true);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 8: SELECT result set
             * Expected: ROW_COUNT() = -1
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1),(2),(3)");

            System.out.println("=================================================");
            System.out.println("SELECT * FROM rowcount_test");
            try (ResultSet rs = stmt.executeQuery("SELECT * FROM rowcount_test"))
            {
                while (rs.next())
                {
                    System.out.println("id = " + rs.getInt(1));
                }
            }

            checkRowCount(stmt, "Case 8: SELECT result set", -1);
            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 9: CREATE TABLE AS SELECT
             * Expected: ROW_COUNT() = copied rows, 3
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute("DROP TABLE IF EXISTS rowcount_test2");

            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1),(2),(3)");

            executeAndCheckRowCount(stmt,
                "CREATE TABLE rowcount_test2 AS SELECT * FROM rowcount_test",
                3, true);

            stmt.execute("DROP TABLE rowcount_test");
            stmt.execute("DROP TABLE rowcount_test2");


            /*
             * Case 10: INSERT INTO ... SELECT
             * Expected: ROW_COUNT() = inserted rows, 3
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute("DROP TABLE IF EXISTS rowcount_test2");

            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");
            stmt.execute(
                "CREATE TABLE rowcount_test2(id INT PRIMARY KEY)");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1),(2),(3)");

            executeAndCheckRowCount(stmt,
                "INSERT INTO rowcount_test2 SELECT * FROM rowcount_test",
                3, true);

            stmt.execute("DROP TABLE rowcount_test");
            stmt.execute("DROP TABLE rowcount_test2");


            /*
             * Case 11: REPLACE insert path
             * Expected: ROW_COUNT() = 1
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, name VARCHAR(20))");

            executeAndCheckRowCount(stmt,
                "REPLACE INTO rowcount_test VALUES (1,'Tom')",
                1, true);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 12: REPLACE replace path
             * Expected: ROW_COUNT() = 2
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, name VARCHAR(20))");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1,'Tom')");

            executeAndCheckRowCount(stmt,
                "REPLACE INTO rowcount_test VALUES (1,'Jerry')",
                2, true);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 13: INSERT ... ON DUPLICATE KEY UPDATE insert path
             * Expected: ROW_COUNT() = 1
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, name VARCHAR(20))");

            executeAndCheckRowCount(stmt,
                "INSERT INTO rowcount_test VALUES (1,'Tom') " +
                "ON DUPLICATE KEY UPDATE name = 'Tom'",
                1, true);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 14: INSERT ... ON DUPLICATE KEY UPDATE update path
             * Expected: ROW_COUNT() = 2
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, name VARCHAR(20))");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1,'Tom')");

            executeAndCheckRowCount(stmt,
                "INSERT INTO rowcount_test VALUES (1,'Jerry') " +
                "ON DUPLICATE KEY UPDATE name = 'Jerry'",
                2, false);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 15: INSERT ... ON DUPLICATE KEY UPDATE no change path
             * Expected: ROW_COUNT() = 0
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY, name VARCHAR(20))");
            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1,'Tom')");

            executeAndCheckRowCount(stmt,
                "INSERT INTO rowcount_test VALUES (1,'Tom') " +
                "ON DUPLICATE KEY UPDATE name = 'Tom'",
                0, false);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 16: Continuous SELECT ROW_COUNT()
             *
             * INSERT -> first ROW_COUNT() = 1
             * first SELECT ROW_COUNT() itself is SELECT,
             * so second ROW_COUNT() = -1
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");

            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1)");

            checkRowCount(stmt,
                "Case 16: first SELECT ROW_COUNT() after INSERT",
                1);

            checkRowCount(stmt,
                "Case 16: second SELECT ROW_COUNT() after first ROW_COUNT()",
                -1);

            stmt.execute("DROP TABLE rowcount_test");


            /*
             * Case 17: Continuous SELECT ROW_COUNT() three times
             *
             * INSERT -> 1
             * ROW_COUNT() -> 1
             * ROW_COUNT() -> -1
             * ROW_COUNT() -> -1
             */
            stmt.execute("DROP TABLE IF EXISTS rowcount_test");
            stmt.execute(
                "CREATE TABLE rowcount_test(id INT PRIMARY KEY)");

            stmt.execute(
                "INSERT INTO rowcount_test VALUES (1)");

            long[] expected = {1, -1, -1};

            for (int i = 0; i < expected.length; i++)
            {
                long actual = queryRowCount(stmt);

                System.out.println("=================================================");
                System.out.println("Case 17: SELECT ROW_COUNT() call " + (i + 1));
                System.out.println("ROW_COUNT() = " + actual +
                    ", expected = " + expected[i]);

                if (actual != expected[i])
                {
                    throw new RuntimeException(
                        "Case 17 failed at call " + (i + 1) +
                        ", expected=" + expected[i] +
                        ", actual=" + actual);
                }
            }

            stmt.execute("DROP TABLE rowcount_test");


            System.out.println("=================================================");
            System.out.println("ALL ROW_COUNT() JDBC TESTS PASSED");
        }
    }

    private static long queryRowCount(Statement stmt)
        throws SQLException
    {
        try (ResultSet rs = stmt.executeQuery("SELECT ROW_COUNT()"))
        {
            rs.next();
            return rs.getLong(1);
        }
    }

    private static void executeAndCheckRowCount(
        Statement stmt,
        String sql,
        long expected,
        boolean flag)
        throws SQLException
    {
        System.out.println("=================================================");
        System.out.println(sql);

        stmt.execute(sql);

        long actual = queryRowCount(stmt);

        System.out.println("ROW_COUNT() = " + actual + ", expected = " + expected);

        if (actual != expected)
        {
            if (flag)
                throw new RuntimeException(
                    "ROW_COUNT check failed, sql=[" + sql + "], expected=" +
                    expected + ", actual=" + actual);
            else
            System.out.println("result is diff, expected=" +
                    expected + ", actual=" + actual);
        }
    }

    private static void checkRowCount(
        Statement stmt,
        String caseName,
        long expected)
        throws SQLException
    {
        long actual = queryRowCount(stmt);

        System.out.println("=================================================");
        System.out.println(caseName);
        System.out.println("ROW_COUNT() = " + actual + ", expected = " + expected);

        if (actual != expected)
        {
            throw new RuntimeException(
                caseName + " failed, expected=" + expected + ", actual=" + actual);
        }
    }
}