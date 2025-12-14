package nim.ui;

import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.layout.*;
import javafx.stage.Stage;
import nim.game.GameState;
import nim.network.Client;
import nim.network.Protocol;
import nim.util.Logger;

import java.util.ArrayList;
import java.util.List;

/**
 * Lobby obrazovka.
 * Zobrazuje seznam mistnosti a umoznuje vytvareni a pripojovani.
 */
public class LobbyView {

    private final Stage stage;
    private final Client client;
    private final GameState gameState;

    // UI komponenty
    private TableView<RoomRow> roomsTable;
    private ObservableList<RoomRow> roomsList;
    private Button refreshButton;
    private Button createButton;
    private Button joinButton;
    private TextField roomNameField;
    private Label statusLabel;
    private Region statusIndicator;
    private Label errorLabel;
    private Label waitingLabel;
    private VBox waitingPane;

    /**
     * Radek tabulky mistnosti.
     */
    public static class RoomRow {
        private final int id;
        private final String name;
        private final String players;
        private final String status;

        public RoomRow(int id, String name, int playerCount, int maxPlayers) {
            this.id = id;
            this.name = name;
            this.players = playerCount + "/" + maxPlayers;
            this.status = playerCount >= maxPlayers ? "Plná" : "Volná";
        }

        public int getId() { return id; }
        public String getName() { return name; }
        public String getPlayers() { return players; }
        public String getStatus() { return status; }
    }

    public LobbyView(Stage stage, Client client, GameState gameState) {
        this.stage = stage;
        this.client = client;
        this.gameState = gameState;
        
        setupMessageHandler();
    }

    /**
     * Zobrazi lobby obrazovku.
     */
    public void show() {
        BorderPane root = Components.createPageLayout();

        // Header
        HBox header = new HBox(20);
        header.setAlignment(Pos.CENTER_LEFT);
        header.setPadding(new Insets(0, 0, 20, 0));
        
        Label title = Components.createHeading("Lobby");
        Region spacer = Components.createSpacer();
        Label userLabel = Components.createText("Hráč: " + gameState.getNickname());
        Button logoutButton = Components.createSecondaryButton("Odhlásit");
        logoutButton.setOnAction(e -> handleLogout());
        
        header.getChildren().addAll(title, spacer, userLabel, logoutButton);

        // Hlavni obsah
        HBox mainContent = new HBox(20);
        mainContent.setAlignment(Pos.TOP_CENTER);
        VBox.setVgrow(mainContent, Priority.ALWAYS);

        // Levy panel - seznam mistnosti
        VBox roomsPanel = Components.createCard();
        roomsPanel.setAlignment(Pos.TOP_CENTER);
        roomsPanel.setPrefWidth(500);
        VBox.setVgrow(roomsPanel, Priority.ALWAYS);
        HBox.setHgrow(roomsPanel, Priority.ALWAYS);

        Label roomsTitle = Components.createText("Herní místnosti");
        roomsTitle.setStyle("-fx-font-weight: bold; -fx-font-size: 16px;");

        // Tabulka
        roomsTable = createRoomsTable();
        roomsList = FXCollections.observableArrayList();
        roomsTable.setItems(roomsList);
        VBox.setVgrow(roomsTable, Priority.ALWAYS);

        // Tlacitka
        HBox roomButtons = new HBox(10);
        roomButtons.setAlignment(Pos.CENTER);
        
        refreshButton = Components.createSecondaryButton("Obnovit");
        refreshButton.setOnAction(e -> handleRefresh());
        
        joinButton = Components.createPrimaryButton("Připojit se");
        joinButton.setOnAction(e -> handleJoinRoom());
        joinButton.setDisable(true);
        
        roomButtons.getChildren().addAll(refreshButton, joinButton);

        // Chybova zprava
        errorLabel = Components.createErrorLabel("");
        errorLabel.setVisible(false);

        roomsPanel.getChildren().addAll(roomsTitle, roomsTable, roomButtons, errorLabel);

        // Pravy panel - vytvoreni mistnosti
        VBox createPanel = Components.createCard();
        createPanel.setAlignment(Pos.TOP_CENTER);
        createPanel.setPrefWidth(300);
        createPanel.setMaxWidth(300);

        Label createTitle = Components.createText("Vytvořit místnost");
        createTitle.setStyle("-fx-font-weight: bold; -fx-font-size: 16px;");

        roomNameField = Components.createTextField("Název místnosti");
        roomNameField.setOnAction(e -> handleCreateRoom());

        createButton = Components.createPrimaryButton("Vytvořit");
        createButton.setMaxWidth(Double.MAX_VALUE);
        createButton.setOnAction(e -> handleCreateRoom());

        // Pravidla hry
        VBox rulesBox = new VBox(5);
        rulesBox.setPadding(new Insets(20, 0, 0, 0));
        
        Label rulesTitle = Components.createText("Pravidla hry");
        rulesTitle.setStyle("-fx-font-weight: bold;");
        
        Label rules = Components.createTextLight(
                "• Začíná se s 21 kamínky\n" +
                "• Střídáte se v odebírání 1-3 kamínků\n" +
                "• Kdo vezme poslední, prohrává\n" +
                "• Každý hráč může 1× přeskočit tah"
        );
        rules.setWrapText(true);
        
        rulesBox.getChildren().addAll(rulesTitle, rules);

        createPanel.getChildren().addAll(createTitle, roomNameField, createButton, rulesBox);

        // Cekaci panel (skryty)
        waitingPane = Components.createCard();
        waitingPane.setAlignment(Pos.CENTER);
        waitingPane.setPrefWidth(300);
        waitingPane.setMaxWidth(300);
        waitingPane.setVisible(false);

        waitingLabel = Components.createText("Čekání na protihráče...");
        ProgressIndicator waitingProgress = new ProgressIndicator();
        waitingProgress.setMaxSize(40, 40);
        
        Button cancelButton = Components.createDangerButton("Zrušit");
        cancelButton.setOnAction(e -> handleLeaveRoom());

        waitingPane.getChildren().addAll(waitingLabel, waitingProgress, cancelButton);

        mainContent.getChildren().addAll(roomsPanel, createPanel, waitingPane);

        // Status bar
        HBox statusBar = Components.createStatusBar();
        statusIndicator = Components.createStatusIndicator(client.isConnected());
        statusLabel = Components.createTextLight(client.isConnected() ? "Připojeno" : "Odpojeno");
        statusBar.getChildren().addAll(statusIndicator, statusLabel);

        root.setTop(header);
        root.setCenter(mainContent);
        root.setBottom(statusBar);

        Scene scene = new Scene(root, 900, 600);
        stage.setScene(scene);

        // Selection listener pro tabulku
        roomsTable.getSelectionModel().selectedItemProperty().addListener(
                (obs, oldVal, newVal) -> joinButton.setDisable(newVal == null));

        // Nacti seznam mistnosti
        handleRefresh();

        Logger.info("Lobby view displayed");
    }

    /**
     * Vytvori tabulku mistnosti.
     */
    @SuppressWarnings("unchecked")
    private TableView<RoomRow> createRoomsTable() {
        TableView<RoomRow> table = new TableView<>();
        table.setPlaceholder(new Label("Žádné místnosti"));
        table.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY);

        TableColumn<RoomRow, String> nameCol = new TableColumn<>("Název");
        nameCol.setCellValueFactory(new PropertyValueFactory<>("name"));
        nameCol.setPrefWidth(200);

        TableColumn<RoomRow, String> playersCol = new TableColumn<>("Hráči");
        playersCol.setCellValueFactory(new PropertyValueFactory<>("players"));
        playersCol.setPrefWidth(80);

        TableColumn<RoomRow, String> statusCol = new TableColumn<>("Stav");
        statusCol.setCellValueFactory(new PropertyValueFactory<>("status"));
        statusCol.setPrefWidth(80);

        table.getColumns().addAll(nameCol, playersCol, statusCol);

        // Double-click = join
        table.setRowFactory(tv -> {
            TableRow<RoomRow> row = new TableRow<>();
            row.setOnMouseClicked(event -> {
                if (event.getClickCount() == 2 && !row.isEmpty()) {
                    handleJoinRoom();
                }
            });
            return row;
        });

        return table;
    }

    /**
     * Zpracuje obnoveni seznamu.
     */
    private void handleRefresh() {
        hideError();
        client.send(Protocol.createListRooms());
    }

    /**
     * Zpracuje vytvoreni mistnosti.
     */
    private void handleCreateRoom() {
        String name = roomNameField.getText().trim();
        
        if (name.isEmpty()) {
            showError("Zadejte název místnosti");
            roomNameField.requestFocus();
            return;
        }
        
        if (!name.matches("^[a-zA-Z0-9_ ]{1,64}$")) {
            showError("Název může obsahovat pouze písmena, čísla, mezery a podtržítka (max 64 znaků)");
            roomNameField.requestFocus();
            return;
        }
        
        hideError();
        client.send(Protocol.createCreateRoom(name));
    }

    /**
     * Zpracuje pripojeni do mistnosti.
     */
    private void handleJoinRoom() {
        RoomRow selected = roomsTable.getSelectionModel().getSelectedItem();
        if (selected == null) {
            showError("Vyberte místnost");
            return;
        }
        
        if ("Plná".equals(selected.getStatus())) {
            showError("Místnost je plná");
            return;
        }
        
        hideError();
        client.send(Protocol.createJoinRoom(selected.getId()));
    }

    /**
     * Zpracuje opusteni mistnosti.
     */
    private void handleLeaveRoom() {
        client.send(Protocol.createLeaveRoom());
    }

    /**
     * Zpracuje odhlaseni.
     */
    private void handleLogout() {
        client.disconnect();
        gameState.reset();
        LoginView loginView = new LoginView(stage);
        loginView.show();
    }

    /**
     * Zobrazi cekaci panel.
     */
    private void showWaitingPane(String roomName) {
        waitingLabel.setText("Čekání na protihráče v místnosti:\n" + roomName);
        waitingPane.setVisible(true);
        createButton.setDisable(true);
        roomNameField.setDisable(true);
        joinButton.setDisable(true);
        refreshButton.setDisable(true);
    }

    /**
     * Skryje cekaci panel.
     */
    private void hideWaitingPane() {
        waitingPane.setVisible(false);
        createButton.setDisable(false);
        roomNameField.setDisable(false);
        refreshButton.setDisable(false);
    }

    /**
     * Zobrazi chybu.
     */
    private void showError(String message) {
        errorLabel.setText(message);
        errorLabel.setVisible(true);
    }

    /**
     * Skryje chybu.
     */
    private void hideError() {
        errorLabel.setVisible(false);
    }

    /**
     * Nastavi message handler.
     */
    private void setupMessageHandler() {
        client.setMessageHandler(message -> {
            Platform.runLater(() -> handleMessage(message));
        });
        
        client.setConnectionStateHandler(state -> {
            Platform.runLater(() -> {
                // Kontrola, zda jsou UI komponenty inicializovány
                if (statusIndicator == null || statusLabel == null) {
                    return;
                }
                
                boolean connected = state == Client.ConnectionState.CONNECTED;
                statusIndicator.setStyle(String.format(
                        "-fx-background-color: %s; -fx-background-radius: 5;",
                        connected ? Components.SUCCESS_COLOR : Components.DANGER_COLOR));
                
                // Disabluj/enabluj ovládací prvky podle stavu spojení (pokud existují)
                if (refreshButton != null && createButton != null && joinButton != null && roomNameField != null) {
                    boolean controlsEnabled = connected;
                    refreshButton.setDisable(!controlsEnabled);
                    createButton.setDisable(!controlsEnabled);
                    joinButton.setDisable(!controlsEnabled);
                    roomNameField.setDisable(!controlsEnabled);
                }
                
                switch (state) {
                    case DISCONNECTED:
                        statusLabel.setText("Odpojeno");
                        break;
                    case CONNECTING:
                        statusLabel.setText("Připojování...");
                        break;
                    case CONNECTED:
                        statusLabel.setText("Připojeno");
                        break;
                    case RECONNECTING:
                        statusLabel.setText("Obnovování spojení...");
                        break;
                }
            });
        });
        
        client.setDisconnectHandler(() -> {
            Platform.runLater(() -> {
                Components.showError("Odpojeno", "Spojení se serverem bylo ztraceno.");
                gameState.reset();
                LoginView loginView = new LoginView(stage);
                loginView.show();
            });
        });
    }

    /**
     * Zpracuje prijatou zpravu.
     */
    private void handleMessage(Protocol.ParsedMessage message) {
        Logger.debug("Lobby handling message: %s", message.getType());
        
        switch (message.getType()) {
            case ROOMS:
                handleRoomsList(message);
                break;
                
            case ROOM_CREATED:
                int roomId = message.getParamAsInt(0);
                gameState.enterRoom(roomId, roomNameField.getText().trim(), null);
                showWaitingPane(roomNameField.getText().trim());
                roomNameField.clear();
                Logger.info("Room created with ID: %d", roomId);
                break;
                
            case ROOM_JOINED:
                handleRoomJoined(message);
                break;
                
            case ROOM_ERR:
                int errorCode = message.getParamAsInt(0);
                Protocol.ErrorCode code = Protocol.ErrorCode.fromCode(errorCode);
                showError(Protocol.getErrorMessage(code));
                break;
                
            case LEAVE_OK:
                gameState.leaveRoom();
                hideWaitingPane();
                handleRefresh();
                break;
                
            case WAIT_OPPONENT:
                // Uz jsme ve waiting pane
                break;
                
            case GAME_START:
                handleGameStart(message);
                break;
                
            case GAME_RESUMED:
                handleGameResumed(message);
                break;
            
            case LOGIN_OK:
                // Server restartoval - jsme znovu přihlášeni
                // Obnov seznam místností
                Logger.info("Server restarted, refreshing lobby");
                gameState.leaveRoom();
                if (waitingPane != null) {
                    hideWaitingPane();
                }
                if (client.isConnected()) {
                    client.send(Protocol.createListRooms());
                }
                break;
                
            case LOGIN_ERR:
                // Chyba při reconnectu - vrať na login
                Logger.warning("Login failed after reconnect: %s", message.getParam(1));
                gameState.reset();
                LoginView loginView = new LoginView(stage);
                loginView.show();
                break;
                
            case ERROR:
                showError("Chyba serveru: " + message.getParam(1));
                break;
                
            default:
                Logger.warning("Unexpected message in lobby: %s", message.getType());
                break;
        }
    }

    /**
     * Zpracuje seznam mistnosti.
     */
    private void handleRoomsList(Protocol.ParsedMessage message) {
        roomsList.clear();
        
        if (message.getParamCount() == 0) return;
        
        int count = message.getParamAsInt(0);
        if (count == 0) return;
        
        List<GameState.RoomInfo> rooms = new ArrayList<>();
        
        // Parametry od indexu 1 jsou mistnosti ve formatu: id,name,players,max
        for (int i = 1; i < message.getParamCount(); i++) {
            String roomData = message.getParam(i);
            String[] parts = roomData.split(",");
            
            if (parts.length >= 4) {
                try {
                    int id = Integer.parseInt(parts[0]);
                    String name = parts[1];
                    int playerCount = Integer.parseInt(parts[2]);
                    int maxPlayers = Integer.parseInt(parts[3]);
                    
                    roomsList.add(new RoomRow(id, name, playerCount, maxPlayers));
                    rooms.add(new GameState.RoomInfo(id, name, playerCount, maxPlayers));
                } catch (NumberFormatException e) {
                    Logger.warning("Invalid room data: %s", roomData);
                }
            }
        }
        
        gameState.setRooms(rooms);
        Logger.debug("Loaded %d rooms", roomsList.size());
    }

    /**
     * Zpracuje pripojeni do mistnosti.
     */
    private void handleRoomJoined(Protocol.ParsedMessage message) {
        int roomId = message.getParamAsInt(0);
        String opponent = message.getParam(1);
        
        // Najdi nazev mistnosti
        String roomName = "";
        for (RoomRow row : roomsList) {
            if (row.getId() == roomId) {
                roomName = row.getName();
                break;
            }
        }
        
        gameState.enterRoom(roomId, roomName, opponent);
        
        if (opponent == null || opponent.isEmpty()) {
            // Cekame na protihrace
            showWaitingPane(roomName);
        }
        // Jinak pockame na GAME_START
    }

    /**
     * Zpracuje zacatek hry.
     */
    private void handleGameStart(Protocol.ParsedMessage message) {
        int stones = message.getParamAsInt(0);
        boolean myTurn = message.getParamAsBoolean(1);
        String opponent = message.getParam(2);
        
        gameState.startGame(stones, myTurn, opponent);
        
        // Prejdi na herni obrazovku
        GameView gameView = new GameView(stage, client, gameState);
        gameView.show();
    }

    /**
     * Zpracuje obnoveni hry (po reconnectu).
     */
    private void handleGameResumed(Protocol.ParsedMessage message) {
        int stones = message.getParamAsInt(0);
        boolean myTurn = message.getParamAsBoolean(1);
        int mySkips = message.getParamAsInt(2);
        int oppSkips = message.getParamAsInt(3);
        
        gameState.resumeGame(stones, myTurn, mySkips, oppSkips);
        
        // Prejdi na herni obrazovku
        GameView gameView = new GameView(stage, client, gameState);
        gameView.show();
    }
}

