import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Properties;

public class MySQLJdbcFetchBinaryTest {
    private static final String TABLE = "test_binary_temporal";
    private static final String SELECT_SQL =
            "SELECT d, t, t6, dt, dt6, ts, ts6 FROM " + TABLE;
    private static final String[] LABELS = {
            "DATE", "TIME", "TIME(6)", "DATETIME", "DATETIME(6)",
            "TIMESTAMP", "TIMESTAMP(6)"
    };

    public static void main(String[] args) throws Exception {
        Properties info = new Properties();

        info.setProperty("HOST", args[0]);
        info.setProperty("PORT", args[1]);
        info.setProperty("DBNAME", args[2]);
        info.setProperty("user", args[3]);
        info.setProperty("password", args[4]);

        String url = "jdbc:mysql://?allowPublicKeyRetrieval=true&useSSL=false"
                + "&useServerPrepStmts=true&cachePrepStmts=false";

        try (Connection connection = DriverManager.getConnection(url, info)) {
            initData(connection);
            printTextAndBinaryResults(connection);
        }
    }

    private static void initData(Connection connection) throws SQLException {
        try (Statement statement = connection.createStatement()) {
            statement.execute("DROP TABLE IF EXISTS " + TABLE);
            statement.execute("CREATE TABLE " + TABLE + " ("
                    + "d DATE, "
                    + "t TIME, "
                    + "t6 TIME(6), "
                    + "dt DATETIME, "
                    + "dt6 DATETIME(6), "
                    + "ts TIMESTAMP, "
                    + "ts6 TIMESTAMP(6) default '2022-1-1 1:1:1')");

            statement.execute("INSERT INTO " + TABLE + " VALUES ("
                    + "'2026-04-29', "
                    + "'12:34:56', "
                    + "'12:34:56.123456', "
                    + "'2026-04-29 04:05:54', "
                    + "'2026-04-29 04:05:54.123456', "
                    + "'2026-04-29 04:05:54', "
                    + "'2026-04-29 04:05:54.123456')");
        }
    }

    private static void printTextAndBinaryResults(Connection connection) throws SQLException {
        String[] textValues;
        String[] binaryValues;

        try (Statement statement = connection.createStatement()) {
            ResultSet resultSet = statement.executeQuery(SELECT_SQL);
            textValues = readRow(resultSet);
        }

        try (PreparedStatement statement = connection.prepareStatement(SELECT_SQL)) {
            ResultSet resultSet = statement.executeQuery();
            binaryValues = readRow(resultSet);
        }

        for (int i = 0; i < LABELS.length; i++) {
            System.out.println(LABELS[i] + " text=" + textValues[i]
                    + " binary=" + binaryValues[i]);
        }
    }

    private static String[] readRow(ResultSet resultSet) throws SQLException {
        String[] values = new String[LABELS.length];

        if (!resultSet.next()) {
            throw new SQLException("temporal test row was not found");
        }
        for (int i = 0; i < LABELS.length; i++) {
            values[i] = resultSet.getString(i + 1);
        }
        return values;
    }
}
