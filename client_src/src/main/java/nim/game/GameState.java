package nim.game;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Model stavu hry.
 * Uchovava vsechny informace o aktualni hre a notifikuje observery o zmenach.
 */
public class GameState {

    /** Pocatecni pocet kaminku */
    public static final int INITIAL_STONES = 21;
    
    /** Minimalni pocet k odebrani */
    public static final int MIN_TAKE = 1;
    
    /** Maximalni pocet k odebrani */
    public static final int MAX_TAKE = 3;
    
    /** Pocet preskoceni na hrace */
    public static final int SKIPS_PER_PLAYER = 1;

    /**
     * Faze hry.
     */
    public enum Phase {
        DISCONNECTED,
        CONNECTING,
        LOGIN,
        LOBBY,
        IN_ROOM_WAITING,
        IN_GAME,
        GAME_OVER
    }

    /**
     * Stav protihrace.
     */
    public enum OpponentStatus {
        CONNECTED,
        DISCONNECTED,
        RECONNECTED
    }

    // Stav pripojeni
    private Phase phase = Phase.DISCONNECTED;
    private String nickname = "";
    private String errorMessage = "";

    // Stav mistnosti
    private int currentRoomId = -1;
    private String currentRoomName = "";
    private List<RoomInfo> rooms = new ArrayList<>();

    // Stav hry
    private int stones = INITIAL_STONES;
    private boolean myTurn = false;
    private int mySkipsRemaining = SKIPS_PER_PLAYER;
    private int opponentSkipsRemaining = SKIPS_PER_PLAYER;
    private String opponentNickname = "";
    private OpponentStatus opponentStatus = OpponentStatus.CONNECTED;
    private String winner = "";
    private String loser = "";

    // Observer
    private Consumer<GameState> stateChangeListener;

    /**
     * Informace o mistnosti.
     */
    public static class RoomInfo {
        public final int id;
        public final String name;
        public final int playerCount;
        public final int maxPlayers;

        public RoomInfo(int id, String name, int playerCount, int maxPlayers) {
            this.id = id;
            this.name = name;
            this.playerCount = playerCount;
            this.maxPlayers = maxPlayers;
        }

        public boolean isFull() {
            return playerCount >= maxPlayers;
        }
    }

    /**
     * Nastavi listener pro zmeny stavu.
     */
    public void setStateChangeListener(Consumer<GameState> listener) {
        this.stateChangeListener = listener;
    }

    /**
     * Notifikuje listener o zmene stavu.
     */
    private void notifyStateChange() {
        if (stateChangeListener != null) {
            stateChangeListener.accept(this);
        }
    }

    // ============================================
    // Gettery
    // ============================================

    public Phase getPhase() {
        return phase;
    }

    public String getNickname() {
        return nickname;
    }

    public String getErrorMessage() {
        return errorMessage;
    }

    public int getCurrentRoomId() {
        return currentRoomId;
    }

    public String getCurrentRoomName() {
        return currentRoomName;
    }

    public List<RoomInfo> getRooms() {
        return new ArrayList<>(rooms);
    }

    public int getStones() {
        return stones;
    }

    public boolean isMyTurn() {
        return myTurn;
    }

    public int getMySkipsRemaining() {
        return mySkipsRemaining;
    }

    public int getOpponentSkipsRemaining() {
        return opponentSkipsRemaining;
    }

    public String getOpponentNickname() {
        return opponentNickname;
    }

    public OpponentStatus getOpponentStatus() {
        return opponentStatus;
    }

    public String getWinner() {
        return winner;
    }

    public String getLoser() {
        return loser;
    }

    public boolean canSkip() {
        return mySkipsRemaining > 0;
    }

    public int getMaxTake() {
        return Math.min(MAX_TAKE, stones);
    }

    // ============================================
    // Settery / akce
    // ============================================

    public void setPhase(Phase phase) {
        this.phase = phase;
        notifyStateChange();
    }

    public void setNickname(String nickname) {
        this.nickname = nickname;
        notifyStateChange();
    }

    public void setErrorMessage(String message) {
        this.errorMessage = message;
        notifyStateChange();
    }

    public void clearError() {
        this.errorMessage = "";
        notifyStateChange();
    }

    public void setRooms(List<RoomInfo> rooms) {
        this.rooms = new ArrayList<>(rooms);
        notifyStateChange();
    }

    public void enterRoom(int roomId, String roomName, String opponent) {
        this.currentRoomId = roomId;
        this.currentRoomName = roomName;
        this.opponentNickname = opponent != null ? opponent : "";
        this.phase = opponent != null && !opponent.isEmpty() ? 
                     Phase.IN_GAME : Phase.IN_ROOM_WAITING;
        notifyStateChange();
    }

    public void leaveRoom() {
        this.currentRoomId = -1;
        this.currentRoomName = "";
        this.opponentNickname = "";
        this.phase = Phase.LOBBY;
        resetGame();
        notifyStateChange();
    }

    public void startGame(int stones, boolean myTurn, String opponent) {
        this.stones = stones;
        this.myTurn = myTurn;
        this.opponentNickname = opponent;
        this.mySkipsRemaining = SKIPS_PER_PLAYER;
        this.opponentSkipsRemaining = SKIPS_PER_PLAYER;
        this.opponentStatus = OpponentStatus.CONNECTED;
        this.winner = "";
        this.loser = "";
        this.phase = Phase.IN_GAME;
        notifyStateChange();
    }

    public void resumeGame(int stones, boolean myTurn, int mySkips, int oppSkips) {
        this.stones = stones;
        this.myTurn = myTurn;
        this.mySkipsRemaining = mySkips;
        this.opponentSkipsRemaining = oppSkips;
        this.opponentStatus = OpponentStatus.CONNECTED;
        this.phase = Phase.IN_GAME;
        notifyStateChange();
    }

    public void updateStones(int remaining, boolean myTurn) {
        this.stones = remaining;
        this.myTurn = myTurn;
        notifyStateChange();
    }

    public void myTakeSucceeded(int remaining, boolean stillMyTurn) {
        this.stones = remaining;
        this.myTurn = stillMyTurn;
        notifyStateChange();
    }

    public void mySkipSucceeded(boolean stillMyTurn) {
        this.mySkipsRemaining--;
        this.myTurn = stillMyTurn;
        notifyStateChange();
    }

    public void opponentTook(int count, int remaining) {
        this.stones = remaining;
        this.myTurn = true;
        notifyStateChange();
    }

    public void opponentSkipped(int remaining) {
        this.opponentSkipsRemaining--;
        this.stones = remaining;
        this.myTurn = true;
        notifyStateChange();
    }

    public void setOpponentStatus(OpponentStatus status) {
        this.opponentStatus = status;
        notifyStateChange();
    }

    public void endGame(String winner, String loser) {
        this.winner = winner;
        this.loser = loser;
        this.phase = Phase.GAME_OVER;
        notifyStateChange();
    }

    public void resetGame() {
        this.stones = INITIAL_STONES;
        this.myTurn = false;
        this.mySkipsRemaining = SKIPS_PER_PLAYER;
        this.opponentSkipsRemaining = SKIPS_PER_PLAYER;
        this.winner = "";
        this.loser = "";
        this.opponentStatus = OpponentStatus.CONNECTED;
    }

    public void reset() {
        this.phase = Phase.DISCONNECTED;
        this.nickname = "";
        this.errorMessage = "";
        this.currentRoomId = -1;
        this.currentRoomName = "";
        this.rooms.clear();
        this.opponentNickname = "";
        resetGame();
        notifyStateChange();
    }

    /**
     * Validuje tah.
     */
    public boolean isValidTake(int count) {
        return count >= MIN_TAKE && count <= MAX_TAKE && count <= stones;
    }
}

