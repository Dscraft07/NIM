package nim.network;

import nim.util.Logger;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.concurrent.*;
import java.util.function.Consumer;

/**
 * Sitovy klient pro komunikaci se serverem.
 * Zajistuje pripojeni, odesilani a prijem zprav.
 */
public class Client {

    /** Timeout pro pripojeni (ms) */
    private static final int CONNECT_TIMEOUT = 10000;
    
    /** Timeout pro cteni (ms) */
    private static final int READ_TIMEOUT = 2000;
    
    /** Interval pro reconnect (ms) */
    private static final int RECONNECT_INTERVAL = 3000;
    
    /** Maximalni pocet pokusu o reconnect */
    private static final int MAX_RECONNECT_ATTEMPTS = 10;
    
    /** Interval pro ping (ms) */
    private static final int PING_INTERVAL = 8000;
    
    /** Timeout pro PONG odpoved (ms) */
    private static final int PONG_TIMEOUT = 15000;

    private String serverHost;
    private int serverPort;
    private String nickname;

    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;

    private ExecutorService receiverThread;
    private ScheduledExecutorService pingScheduler;
    
    private Consumer<Protocol.ParsedMessage> messageHandler;
    private Consumer<ConnectionState> connectionStateHandler;
    private Runnable disconnectHandler;

    private volatile boolean connected = false;
    private volatile boolean reconnecting = false;
    private volatile int reconnectAttempts = 0;
    private volatile long lastPingTime = 0;
    private volatile boolean waitingForPong = false;
    private volatile int invalidMessageCount = 0;

    /**
     * Stavy pripojeni.
     */
    public enum ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING
    }

    /**
     * Vytvori novou instanci klienta.
     */
    public Client() {
        this.receiverThread = Executors.newSingleThreadExecutor();
        this.pingScheduler = Executors.newSingleThreadScheduledExecutor();
    }

    /**
     * Nastavi handler pro prichozi zpravy.
     */
    public void setMessageHandler(Consumer<Protocol.ParsedMessage> handler) {
        this.messageHandler = handler;
    }

    /**
     * Nastavi handler pro zmenu stavu pripojeni.
     */
    public void setConnectionStateHandler(Consumer<ConnectionState> handler) {
        this.connectionStateHandler = handler;
    }

    /**
     * Nastavi handler pro odpojeni.
     */
    public void setDisconnectHandler(Runnable handler) {
        this.disconnectHandler = handler;
    }

    /**
     * Pripoji se k serveru.
     */
    public boolean connect(String host, int port, String nickname) {
        this.serverHost = host;
        this.serverPort = port;
        this.nickname = nickname;

        notifyConnectionState(ConnectionState.CONNECTING);
        
        try {
            socket = new Socket();
            socket.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT);
            socket.setSoTimeout(READ_TIMEOUT);
            
            // TCP keepalive pro detekci odpojeni
            socket.setKeepAlive(true);
            
            // Zabranit Nagle algoritmu pro rychlejsi odezvu
            socket.setTcpNoDelay(true);
            
            // Vetsi buffer pro prijem
            socket.setReceiveBufferSize(8192);
            socket.setSendBufferSize(8192);
            
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), "UTF-8"));
            writer = new PrintWriter(new OutputStreamWriter(socket.getOutputStream(), "UTF-8"), true);
            
            connected = true;
            reconnectAttempts = 0;
            invalidMessageCount = 0;
            
            // Spust prijimaci vlakno
            startReceiver();
            
            // Spust ping scheduler
            startPingScheduler();
            
            Logger.info("Connected to server %s:%d", host, port);
            notifyConnectionState(ConnectionState.CONNECTED);
            
            // Posli LOGIN
            send(Protocol.createLogin(nickname));
            
            return true;
            
        } catch (IOException e) {
            Logger.error("Failed to connect to server", e);
            notifyConnectionState(ConnectionState.DISCONNECTED);
            return false;
        }
    }

    /**
     * Odpoji se od serveru.
     */
    public void disconnect() {
        disconnect(true);
    }

    /**
     * Odpoji se od serveru.
     */
    private void disconnect(boolean sendLogout) {
        if (sendLogout && connected) {
            try {
                send(Protocol.createLogout());
            } catch (Exception e) {
                // Ignoruj
            }
        }
        
        connected = false;
        reconnecting = false;
        
        stopPingScheduler();
        stopReceiver();
        
        closeSocket();
        
        notifyConnectionState(ConnectionState.DISCONNECTED);
        Logger.info("Disconnected from server");
    }

    /**
     * Odesle zpravu na server.
     */
    public synchronized boolean send(String message) {
        if (!connected || writer == null) {
            Logger.warning("Cannot send message - not connected");
            return false;
        }
        
        try {
            writer.print(message);
            writer.flush();
            
            if (writer.checkError()) {
                throw new IOException("Write error");
            }
            
            // Log bez koncoveho \n
            String logMsg = message.trim();
            Logger.debug("Sent: %s", logMsg);
            return true;
            
        } catch (Exception e) {
            Logger.error("Failed to send message", e);
            handleConnectionLost();
            return false;
        }
    }

    /**
     * Zjisti, zda je klient pripojen.
     */
    public boolean isConnected() {
        return connected && socket != null && socket.isConnected() && !socket.isClosed();
    }

    /**
     * Vrati prezdivku.
     */
    public String getNickname() {
        return nickname;
    }

    // ============================================
    // Privatni metody
    // ============================================

    /**
     * Spusti prijimaci vlakno.
     */
    private void startReceiver() {
        receiverThread.submit(() -> {
            Logger.debug("Receiver thread started");
            
            while (connected && !Thread.currentThread().isInterrupted()) {
                try {
                    String line = reader.readLine();
                    
                    if (line == null) {
                        // Server zavrel spojeni
                        Logger.warning("Server closed connection");
                        handleConnectionLost();
                        break;
                    }
                    
                    if (!line.isEmpty()) {
                        Logger.debug("Received: %s", line);
                        processMessage(line);
                    }
                    
                } catch (SocketTimeoutException e) {
                    // Timeout - to je ok, kontrolujeme connected flag
                    continue;
                } catch (IOException e) {
                    if (connected) {
                        Logger.error("Read error", e);
                        handleConnectionLost();
                    }
                    break;
                }
            }
            
            Logger.debug("Receiver thread stopped");
        });
    }

    /**
     * Zastavi prijimaci vlakno.
     */
    private void stopReceiver() {
        receiverThread.shutdownNow();
        try {
            receiverThread.awaitTermination(1, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        receiverThread = Executors.newSingleThreadExecutor();
    }

    /**
     * Spusti ping scheduler.
     */
    private void startPingScheduler() {
        pingScheduler.scheduleAtFixedRate(() -> {
            try {
                if (connected && !waitingForPong) {
                    if (send(Protocol.createPing())) {
                        lastPingTime = System.currentTimeMillis();
                        waitingForPong = true;
                    }
                } else if (waitingForPong) {
                    // Kontrola PONG timeoutu - pouzij delsi timeout
                    long elapsed = System.currentTimeMillis() - lastPingTime;
                    if (elapsed > PONG_TIMEOUT) {
                        Logger.warning("PONG timeout after %d ms", elapsed);
                        handleConnectionLost();
                    }
                }
            } catch (Exception e) {
                Logger.error("Ping scheduler error", e);
            }
        }, PING_INTERVAL, PING_INTERVAL / 2, TimeUnit.MILLISECONDS);
    }

    /**
     * Zastavi ping scheduler.
     */
    private void stopPingScheduler() {
        pingScheduler.shutdownNow();
        try {
            pingScheduler.awaitTermination(1, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        pingScheduler = Executors.newSingleThreadScheduledExecutor();
    }

    /**
     * Zpracuje prijatou zpravu.
     */
    private void processMessage(String rawMessage) {
        // Validace zpravy
        String validationError = Protocol.validateMessage(rawMessage);
        if (validationError != null) {
            invalidMessageCount++;
            Logger.warning("Invalid message from server (%d/%d): %s", 
                          invalidMessageCount, Protocol.MAX_INVALID_MESSAGES, validationError);
            
            if (invalidMessageCount >= Protocol.MAX_INVALID_MESSAGES) {
                Logger.error("Too many invalid messages from server, disconnecting");
                disconnect(false);
                if (disconnectHandler != null) {
                    disconnectHandler.run();
                }
            }
            return;
        }
        
        Protocol.ParsedMessage message = Protocol.parse(rawMessage);
        
        // Kontrola UNKNOWN typu (nevalidni prikaz)
        if (message.getType() == Protocol.MessageType.UNKNOWN) {
            invalidMessageCount++;
            Logger.warning("Unknown message type from server (%d/%d): %s", 
                          invalidMessageCount, Protocol.MAX_INVALID_MESSAGES, rawMessage);
            
            if (invalidMessageCount >= Protocol.MAX_INVALID_MESSAGES) {
                Logger.error("Too many invalid messages from server, disconnecting");
                disconnect(false);
                if (disconnectHandler != null) {
                    disconnectHandler.run();
                }
            }
            return;
        }
        
        // Validni zprava - reset pocitadla
        invalidMessageCount = 0;
        
        // Zpracuj PONG lokalne
        if (message.getType() == Protocol.MessageType.PONG) {
            waitingForPong = false;
            return;
        }
        
        // Zpracuj PING
        if (message.getType() == Protocol.MessageType.PING) {
            send(Protocol.createPong());
            return;
        }
        
        // Zpracuj SERVER_SHUTDOWN
        if (message.getType() == Protocol.MessageType.SERVER_SHUTDOWN) {
            Logger.info("Server is shutting down");
            disconnect(false);
            if (disconnectHandler != null) {
                disconnectHandler.run();
            }
            return;
        }
        
        // Predej handleru
        if (messageHandler != null) {
            messageHandler.accept(message);
        }
    }

    /**
     * Zpracuje ztratu spojeni.
     */
    private void handleConnectionLost() {
        if (reconnecting) return;
        
        Logger.warning("Connection lost");
        connected = false;
        closeSocket();
        
        // Zkus reconnect
        attemptReconnect();
    }

    /**
     * Pokusi se o reconnect.
     */
    private void attemptReconnect() {
        if (reconnecting || reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            Logger.error("Max reconnect attempts reached (%d), giving up", reconnectAttempts);
            reconnecting = false;
            notifyConnectionState(ConnectionState.DISCONNECTED);
            if (disconnectHandler != null) {
                disconnectHandler.run();
            }
            return;
        }
        
        reconnecting = true;
        reconnectAttempts++;
        
        notifyConnectionState(ConnectionState.RECONNECTING);
        Logger.info("Attempting reconnect %d/%d in %d ms...", 
                    reconnectAttempts, MAX_RECONNECT_ATTEMPTS, RECONNECT_INTERVAL);
        
        // Spust reconnect ve vlastnim vlakne
        CompletableFuture.runAsync(() -> {
            try {
                // Exponencialni backoff - cekej dele s kazdym pokusem
                long waitTime = RECONNECT_INTERVAL * Math.min(reconnectAttempts, 3);
                Thread.sleep(waitTime);
                
                if (!reconnecting) return;
                
                // Zavri stary socket, pokud existuje
                closeSocket();
                
                socket = new Socket();
                socket.connect(new InetSocketAddress(serverHost, serverPort), CONNECT_TIMEOUT);
                socket.setSoTimeout(READ_TIMEOUT);
                socket.setKeepAlive(true);
                socket.setTcpNoDelay(true);
                socket.setReceiveBufferSize(8192);
                socket.setSendBufferSize(8192);
                
                reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), "UTF-8"));
                writer = new PrintWriter(new OutputStreamWriter(socket.getOutputStream(), "UTF-8"), true);
                
                connected = true;
                reconnecting = false;
                reconnectAttempts = 0;
                waitingForPong = false;
                invalidMessageCount = 0;
                
                // Spust prijimaci vlakno
                startReceiver();
                
                // Spust ping scheduler
                startPingScheduler();
                
                Logger.info("Reconnected to server successfully");
                notifyConnectionState(ConnectionState.CONNECTED);
                
                // Posli LOGIN pro reconnect
                send(Protocol.createLogin(nickname));
                
            } catch (InterruptedException e) {
                Logger.info("Reconnect interrupted");
                reconnecting = false;
                Thread.currentThread().interrupt();
            } catch (Exception e) {
                Logger.warning("Reconnect attempt %d failed: %s", reconnectAttempts, e.getMessage());
                reconnecting = false;
                // Zkus znovu
                attemptReconnect();
            }
        });
    }

    /**
     * Zavre socket.
     */
    private void closeSocket() {
        try {
            if (reader != null) reader.close();
            if (writer != null) writer.close();
            if (socket != null && !socket.isClosed()) socket.close();
        } catch (IOException e) {
            // Ignoruj
        }
        reader = null;
        writer = null;
        socket = null;
    }

    /**
     * Notifikuje zmenu stavu pripojeni.
     */
    private void notifyConnectionState(ConnectionState state) {
        if (connectionStateHandler != null) {
            connectionStateHandler.accept(state);
        }
    }

    /**
     * Ukonci klienta a uvolni zdroje.
     */
    public void shutdown() {
        disconnect(false);
        receiverThread.shutdownNow();
        pingScheduler.shutdownNow();
    }
}

