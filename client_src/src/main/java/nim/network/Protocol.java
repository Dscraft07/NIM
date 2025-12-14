package nim.network;

import nim.util.Logger;
import java.util.HashMap;
import java.util.Map;

/**
 * Trida pro praci s protokolem komunikace se serverem.
 * Zpracovava a vytvari zpravy podle definovaneho protokolu.
 */
public class Protocol {

    /** Oddelovac parametru ve zprave */
    public static final String DELIMITER = ";";
    
    /** Ukoncovac zpravy */
    public static final String TERMINATOR = "\n";

    /**
     * Typy zprav.
     */
    public enum MessageType {
        // Klientske zpravy
        LOGIN, LIST_ROOMS, CREATE_ROOM, JOIN_ROOM, LEAVE_ROOM,
        TAKE, SKIP, PING, LOGOUT,
        
        // Serverove zpravy
        LOGIN_OK, LOGIN_ERR, ROOMS, ROOM_CREATED, ROOM_JOINED, ROOM_ERR,
        LEAVE_OK, GAME_START, TAKE_OK, TAKE_ERR, SKIP_OK, SKIP_ERR,
        OPPONENT_ACTION, GAME_OVER, PONG, PLAYER_STATUS, ERROR,
        SERVER_SHUTDOWN, WAIT_OPPONENT, GAME_RESUMED,
        
        // Specialni
        UNKNOWN
    }

    /**
     * Chybove kody.
     */
    public enum ErrorCode {
        NONE(0),
        INVALID_FORMAT(1),
        UNKNOWN_COMMAND(2),
        INVALID_PARAMS(3),
        NOT_LOGGED_IN(4),
        ALREADY_LOGGED_IN(5),
        NICKNAME_TAKEN(6),
        NICKNAME_INVALID(7),
        ROOM_NOT_FOUND(8),
        ROOM_FULL(9),
        ROOM_NAME_TAKEN(10),
        NOT_IN_ROOM(11),
        NOT_IN_GAME(12),
        NOT_YOUR_TURN(13),
        INVALID_MOVE(14),
        NO_SKIPS_LEFT(15),
        SERVER_FULL(16),
        MAX_ROOMS(17),
        GAME_IN_PROGRESS(18),
        INTERNAL(99);

        private final int code;
        private static final Map<Integer, ErrorCode> codeMap = new HashMap<>();

        static {
            for (ErrorCode ec : values()) {
                codeMap.put(ec.code, ec);
            }
        }

        ErrorCode(int code) {
            this.code = code;
        }

        public int getCode() {
            return code;
        }

        public static ErrorCode fromCode(int code) {
            return codeMap.getOrDefault(code, INTERNAL);
        }
    }

    /**
     * Parsovana zprava.
     */
    public static class ParsedMessage {
        private MessageType type;
        private String[] params;
        private String raw;

        public ParsedMessage(MessageType type, String[] params, String raw) {
            this.type = type;
            this.params = params;
            this.raw = raw;
        }

        public MessageType getType() {
            return type;
        }

        public String[] getParams() {
            return params;
        }

        public String getParam(int index) {
            if (index >= 0 && index < params.length) {
                return params[index];
            }
            return "";
        }

        public int getParamAsInt(int index) {
            try {
                return Integer.parseInt(getParam(index));
            } catch (NumberFormatException e) {
                return 0;
            }
        }

        public boolean getParamAsBoolean(int index) {
            String param = getParam(index);
            return "1".equals(param) || "true".equalsIgnoreCase(param);
        }

        public int getParamCount() {
            return params.length;
        }

        public String getRaw() {
            return raw;
        }

        @Override
        public String toString() {
            return String.format("Message{type=%s, params=%d, raw='%s'}", 
                    type, params.length, raw);
        }
    }

    /**
     * Parsuje prijatou zpravu.
     */
    public static ParsedMessage parse(String rawMessage) {
        if (rawMessage == null || rawMessage.isEmpty()) {
            return new ParsedMessage(MessageType.UNKNOWN, new String[0], rawMessage);
        }

        // Odstran \r\n
        String message = rawMessage.trim();

        // Rozdel na casti
        String[] parts = message.split(DELIMITER, -1);
        if (parts.length == 0) {
            return new ParsedMessage(MessageType.UNKNOWN, new String[0], rawMessage);
        }

        // Prvni cast je typ zpravy
        MessageType type;
        try {
            type = MessageType.valueOf(parts[0].toUpperCase());
        } catch (IllegalArgumentException e) {
            Logger.warning("Unknown message type: %s", parts[0]);
            type = MessageType.UNKNOWN;
        }

        // Ostatni casti jsou parametry
        String[] params = new String[parts.length - 1];
        System.arraycopy(parts, 1, params, 0, params.length);

        return new ParsedMessage(type, params, rawMessage);
    }

    // ============================================
    // Tvorba klientskych zprav
    // ============================================

    public static String createLogin(String nickname) {
        return "LOGIN" + DELIMITER + nickname + TERMINATOR;
    }

    public static String createListRooms() {
        return "LIST_ROOMS" + TERMINATOR;
    }

    public static String createCreateRoom(String name) {
        return "CREATE_ROOM" + DELIMITER + name + TERMINATOR;
    }

    public static String createJoinRoom(int roomId) {
        return "JOIN_ROOM" + DELIMITER + roomId + TERMINATOR;
    }

    public static String createLeaveRoom() {
        return "LEAVE_ROOM" + TERMINATOR;
    }

    public static String createTake(int count) {
        return "TAKE" + DELIMITER + count + TERMINATOR;
    }

    public static String createSkip() {
        return "SKIP" + TERMINATOR;
    }

    public static String createPing() {
        return "PING" + TERMINATOR;
    }

    public static String createPong() {
        return "PONG" + TERMINATOR;
    }

    public static String createLogout() {
        return "LOGOUT" + TERMINATOR;
    }

    /**
     * Vrati textovy popis chyboveho kodu.
     */
    public static String getErrorMessage(ErrorCode code) {
        switch (code) {
            case NONE: return "OK";
            case INVALID_FORMAT: return "Neplatný formát zprávy";
            case UNKNOWN_COMMAND: return "Neznámý příkaz";
            case INVALID_PARAMS: return "Neplatné parametry";
            case NOT_LOGGED_IN: return "Nejste přihlášen";
            case ALREADY_LOGGED_IN: return "Již jste přihlášen";
            case NICKNAME_TAKEN: return "Přezdívka je obsazena";
            case NICKNAME_INVALID: return "Neplatná přezdívka";
            case ROOM_NOT_FOUND: return "Místnost neexistuje";
            case ROOM_FULL: return "Místnost je plná";
            case ROOM_NAME_TAKEN: return "Název místnosti je obsazen";
            case NOT_IN_ROOM: return "Nejste v místnosti";
            case NOT_IN_GAME: return "Nejste ve hře";
            case NOT_YOUR_TURN: return "Nejste na tahu";
            case INVALID_MOVE: return "Neplatný tah";
            case NO_SKIPS_LEFT: return "Nemáte žádné přeskočení";
            case SERVER_FULL: return "Server je plný";
            case MAX_ROOMS: return "Dosažen limit místností";
            case GAME_IN_PROGRESS: return "Hra již probíhá";
            case INTERNAL: return "Interní chyba serveru";
            default: return "Neznámá chyba";
        }
    }
}

