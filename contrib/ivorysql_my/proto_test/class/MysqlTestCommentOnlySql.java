import java.io.*;
import java.sql.*;
import java.util.*;
import java.security.*;
import java.nio.file.*;

public class MysqlTestCommentOnlySql {
    public static void main(String[] args) throws Exception {
        String url = "jdbc:mysql:///?allowPublicKeyRetrieval=true&useSSL=false";
        String COMMENT_ONLY_SQL = "/* comment-only SQL test */";
        String COMMENT_WITH_QUERY_SQL = "/* comment-with-query SQL test */ SELECT 1";

        Properties p = new Properties();
        p.setProperty("HOST", args[0]);
        p.setProperty("PORT", args[1]);
        p.setProperty("DBNAME", args[2]);
        p.setProperty("user", args[3]);
        p.setProperty("password", args[4]);

        try (Connection conn = DriverManager.getConnection(url, p);
             Statement stmt = conn.createStatement()) {

            stmt.setQueryTimeout(10);
            executeAndPrint(stmt, "comment-with-query", COMMENT_WITH_QUERY_SQL);
            executeAndPrint(stmt, "comment-only", COMMENT_ONLY_SQL);
        } catch (SQLException e) {
            System.err.println("Connection or SQL execution failed: " + e.getMessage());
            e.printStackTrace(System.err);
        }
    }

    private static void executeAndPrint(Statement statement, String label, String sql)
            throws SQLException {

        boolean hasResultSet = statement.execute(sql);
        System.out.println(label + " returned; hasResultSet = " + hasResultSet);

        if (hasResultSet) {
            try (ResultSet resultSet = statement.getResultSet()) {
                while (resultSet.next()) {
                    System.out.println(label + " result = " + resultSet.getObject(1));
                }
            }
        } else {
            System.out.println(label + " updateCount = " + statement.getUpdateCount());
        }
    }
}
