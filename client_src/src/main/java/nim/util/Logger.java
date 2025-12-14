package nim.util;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

/**
 * Jednoduchy logger pro klientskou aplikaci.
 * Loguje do souboru i na konzoli.
 */
public class Logger {

    private static final String LOG_FILE = "nim_client.log";
    private static final DateTimeFormatter FORMATTER = 
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
    
    private static PrintWriter fileWriter;
    private static LogLevel minLevel = LogLevel.INFO;

    /**
     * Urovne logovani.
     */
    public enum LogLevel {
        DEBUG, INFO, WARNING, ERROR
    }

    static {
        try {
            fileWriter = new PrintWriter(new FileWriter(LOG_FILE, true), true);
        } catch (IOException e) {
            System.err.println("Warning: Cannot open log file, using console only");
            fileWriter = null;
        }
    }

    /**
     * Nastavi minimalani uroven logovani.
     */
    public static void setLevel(LogLevel level) {
        minLevel = level;
    }

    /**
     * Zaloguje zpravu na dane urovni.
     */
    public static void log(LogLevel level, String message) {
        if (level.ordinal() < minLevel.ordinal()) {
            return;
        }

        String timestamp = LocalDateTime.now().format(FORMATTER);
        String formattedMessage = String.format("[%s] [%s] %s", 
                timestamp, level.name(), message);

        // Konzole
        if (level == LogLevel.ERROR || level == LogLevel.WARNING) {
            System.err.println(formattedMessage);
        } else {
            System.out.println(formattedMessage);
        }

        // Soubor
        if (fileWriter != null) {
            fileWriter.println(formattedMessage);
        }
    }

    /**
     * Zaloguje zpravu s formatovanim.
     */
    public static void log(LogLevel level, String format, Object... args) {
        log(level, String.format(format, args));
    }

    public static void debug(String message) {
        log(LogLevel.DEBUG, message);
    }

    public static void debug(String format, Object... args) {
        log(LogLevel.DEBUG, format, args);
    }

    public static void info(String message) {
        log(LogLevel.INFO, message);
    }

    public static void info(String format, Object... args) {
        log(LogLevel.INFO, format, args);
    }

    public static void warning(String message) {
        log(LogLevel.WARNING, message);
    }

    public static void warning(String format, Object... args) {
        log(LogLevel.WARNING, format, args);
    }

    public static void error(String message) {
        log(LogLevel.ERROR, message);
    }

    public static void error(String format, Object... args) {
        log(LogLevel.ERROR, format, args);
    }

    public static void error(String message, Throwable throwable) {
        log(LogLevel.ERROR, message + ": " + throwable.getMessage());
        if (fileWriter != null) {
            throwable.printStackTrace(fileWriter);
        }
    }

    /**
     * Zavre logger.
     */
    public static void close() {
        if (fileWriter != null) {
            fileWriter.close();
        }
    }
}

