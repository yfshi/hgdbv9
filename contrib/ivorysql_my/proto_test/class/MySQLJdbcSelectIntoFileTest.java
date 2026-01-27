import java.io.*;
import java.nio.*;
import java.sql.*;
import java.util.*;
import java.nio.file.Files;
import java.nio.file.Path;

public class MySQLJdbcSelectIntoFileTest {

	private static String host;
	private static String port = "3307";
	private static String dbname;
	private static String user;
	private static String password;
	private static String jar_version = "old";
	private static String URL;

	public static void main(String[] args) {
		Connection conn = null;

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

		try {
			Class.forName("com.mysql.jdbc.Driver");

			Properties props = new Properties();
			props.setProperty("HOST", host);
			props.setProperty("PORT", port);
			props.setProperty("DBNAME", dbname);
			props.setProperty("user", user);
			props.setProperty("password", password);

			if (jar_version.equals("new"))
			{
				URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
			}
			else
			{
				URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
			}

			conn = DriverManager.getConnection(URL, props);

			createTable(conn);

			insertIntoData(conn);

			selectIntoData(conn);

		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if (conn != null) conn.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	private static void createTable(Connection conn) throws SQLException {
		String createTableSQL =
			"CREATE TABLE IF NOT EXISTS t_select_into(" +
			"id INT, " +
			"age INT, " +
			"name VARCHAR(255))";

		try (Statement stmt = conn.createStatement()) {
			stmt.execute("drop table if exists t_select_into");
			stmt.execute(createTableSQL);
			System.out.println("CREATE TABLE t_select_into SUCCESS");
		}
	}

	private static void insertIntoData(Connection conn) throws SQLException, IOException {

		System.out.println("Insert data (1,1,'highgo')");
		System.out.println("Insert data (2,2,'wor$k')");
		System.out.println("Insert data (3,3,'high,go')");

		try (Statement stmt = conn.createStatement()) {
			stmt.execute("INSERT INTO t_select_into VALUES(1,1,'highgo')");
			stmt.execute("INSERT INTO t_select_into VALUES(2,2,'wor$k')");
			stmt.execute("INSERT INTO t_select_into VALUES(3,3,'high,go')");
			System.out.println("Insert data SUCCESS");
		}
	}

	private static void selectIntoData(Connection conn) throws SQLException, IOException {
		String workerDir = System.getProperty("user.dir");
		File csv = new File(workerDir, "select_into_1.csv");

		String insertSQL = "SELECT * INTO OUTFILE '" + csv.getAbsolutePath()
					+ "' " + "FROM t_select_into";

		try (Statement stmt = conn.createStatement()) {
			stmt.execute(insertSQL);
		}

		List<String> lines = Files.readAllLines(csv.toPath());
		System.out.println("Read the outfile");
		for (String line : lines ) {
			System.out.println(line);
		}

		if (csv.exists()) {
			boolean ok = csv.delete();
			if (!ok)
			{
				System.out.println("Failed to delete the CSV file" + csv.getAbsolutePath());
			}
			else
			{
				System.out.println("Succesed to delete the CSV file");
			}
		}
	}
}
