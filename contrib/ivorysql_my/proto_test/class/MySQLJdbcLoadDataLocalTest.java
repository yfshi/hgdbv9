import java.io.File;
import java.io.FileWriter;
import java.sql.*;
import java.util.Properties;
import java.math.BigDecimal;

public class MySQLJdbcLoadDataLocalTest {
	private static String host;
	private static String port = "3307";
	private static String dbname;
	private static String user;
	private static String password;
	private static String jar_version = "old";
	private static String url_jdbc;

	public static void main(String[] args) throws Exception {
		if (args.length >= 4) {
			host = args[0];
			port = args[1];
			dbname = args[2];
			user = args[3];
		}

		Properties info = new Properties();
		info.setProperty("HOST", host);
		info.setProperty("PORT", port);
		info.setProperty("DBNAME", dbname);
		info.setProperty("user", user);

		if (jar_version.equals("new"))
		{
			url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&createDatabaseIfNotExist=true";
		}
		else
		{
			url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false&createDatabaseIfNotExist=true";
		}

		Connection conn = null;

		try {
			Class.forName("com.mysql.jdbc.Driver");
			conn = DriverManager.getConnection(url_jdbc, info);
			System.out.println("Database connection successful");

			testLoadDataLocal(conn);

		} catch (Exception e) {
			System.err.println("Test failed: " + e.getMessage());
			e.printStackTrace();
		} finally {
			try {
				if (conn != null) conn.close();
					System.out.println("\nDatabase connection closed");
			} catch (SQLException e) {
				System.err.println("Resource cleanup failed: " + e.getMessage());
			}
		}
	}

	private static void testLoadDataLocal(Connection conn) throws Exception {
		Statement statement = conn.createStatement();
		statement.executeUpdate("drop table if exists t_load");
		statement.executeUpdate("create table t_load (" +
							"id int," +
							"name varchar(10)," +
							"PRIMARY KEY (id)" +
							")");

		String workerDir = System.getProperty("user.dir");
		File csv = new File(workerDir, "t_load.csv");

		try (FileWriter fw = new FileWriter(csv)) {
			fw.write("1,highgo\n");
			fw.write("2,wor$k\n");
			fw.write("3,\"high,go\"\n");
		}

		System.out.println("Success generated the CSV file");

		String sql = "LOAD DATA LOCAL INFILE '" + csv.getAbsolutePath() + "' "
							+ "INTO TABLE t_load "
							+ "FIELDS TERMINATED BY ','";

		statement.execute(sql);

		ResultSet rs = statement.executeQuery("SELECT * FROM t_load");
		while (rs.next()) {
			System.out.println(rs.getInt("id") + "," + rs.getString("name"));
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
