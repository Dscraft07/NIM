package nim.ui;

import javafx.animation.*;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.shape.Circle;
import javafx.stage.Stage;
import javafx.util.Duration;
import nim.game.GameState;
import nim.network.Client;
import nim.network.Protocol;
import nim.util.Logger;

import java.util.ArrayList;
import java.util.List;

/**
 * Herni obrazovka.
 * Zobrazuje herni pole, kameny a ovladaci prvky.
 */
public class GameView {

    private final Stage stage;
    private final Client client;
    private final GameState gameState;

    // UI komponenty
    private FlowPane stonesPane;
    private Label stonesCountLabel;
    private Label turnLabel;
    private Label opponentLabel;
    private Label opponentStatusLabel;
    private Label mySkipsLabel;
    private Label oppSkipsLabel;
    private Spinner<Integer> takeSpinner;
    private Button takeButton;
    private Button skipButton;
    private Label skipNoteLabel;
    private Label statusLabel;
    private Region statusIndicator;
    private VBox gameOverPane;
    private Label gameOverTitle;
    private Label gameOverMessage;

    private List<Circle> stoneCircles = new ArrayList<>();

    public GameView(Stage stage, Client client, GameState gameState) {
        this.stage = stage;
        this.client = client;
        this.gameState = gameState;
        
        setupMessageHandler();
        setupStateListener();
    }

    /**
     * Zobrazi herni obrazovku.
     */
    public void show() {
        BorderPane root = Components.createPageLayout();

        // Header
        HBox header = new HBox(20);
        header.setAlignment(Pos.CENTER_LEFT);
        header.setPadding(new Insets(0, 0, 20, 0));
        
        Label title = Components.createHeading("Nim");
        Region spacer = Components.createSpacer();
        
        // Info o hracich
        VBox playersInfo = new VBox(5);
        playersInfo.setAlignment(Pos.CENTER_RIGHT);
        
        Label myLabel = Components.createText("Vy: " + gameState.getNickname());
        opponentLabel = Components.createText("Soupeř: " + gameState.getOpponentNickname());
        opponentStatusLabel = Components.createTextLight("");
        updateOpponentStatus();
        
        playersInfo.getChildren().addAll(myLabel, opponentLabel, opponentStatusLabel);
        
        Button leaveButton = Components.createDangerButton("Opustit hru");
        leaveButton.setOnAction(e -> handleLeaveGame());
        
        header.getChildren().addAll(title, spacer, playersInfo, leaveButton);

        // Hlavni obsah
        VBox mainContent = new VBox(30);
        mainContent.setAlignment(Pos.CENTER);
        VBox.setVgrow(mainContent, Priority.ALWAYS);

        // Informacni panel
        HBox infoPanel = new HBox(40);
        infoPanel.setAlignment(Pos.CENTER);
        
        VBox turnBox = new VBox(5);
        turnBox.setAlignment(Pos.CENTER);
        turnLabel = Components.createHeading(gameState.isMyTurn() ? "Jste na tahu!" : "Soupeř je na tahu");
        turnLabel.setTextFill(Color.web(gameState.isMyTurn() ? 
                Components.SUCCESS_COLOR : Components.TEXT_LIGHT));
        turnBox.getChildren().add(turnLabel);
        
        VBox stonesBox = new VBox(5);
        stonesBox.setAlignment(Pos.CENTER);
        stonesCountLabel = Components.createTitle(String.valueOf(gameState.getStones()));
        Label stonesLabel = Components.createTextLight("kamínků zbývá");
        stonesBox.getChildren().addAll(stonesCountLabel, stonesLabel);
        
        VBox skipsBox = new VBox(5);
        skipsBox.setAlignment(Pos.CENTER);
        mySkipsLabel = Components.createText("Vaše přeskočení: " + gameState.getMySkipsRemaining());
        oppSkipsLabel = Components.createText("Soupeřova přeskočení: " + gameState.getOpponentSkipsRemaining());
        skipsBox.getChildren().addAll(mySkipsLabel, oppSkipsLabel);
        
        infoPanel.getChildren().addAll(turnBox, stonesBox, skipsBox);

        // Vizualizace kamenu
        VBox stonesCard = Components.createCard();
        stonesCard.setMinHeight(200);
        stonesCard.setMaxWidth(600);
        
        stonesPane = new FlowPane(10, 10);
        stonesPane.setAlignment(Pos.CENTER);
        stonesPane.setPrefWrapLength(500);
        
        createStoneCircles(gameState.getStones());
        
        stonesCard.getChildren().add(stonesPane);

        // Ovladaci panel
        HBox controlsPanel = new HBox(20);
        controlsPanel.setAlignment(Pos.CENTER);
        
        VBox takeBox = new VBox(10);
        takeBox.setAlignment(Pos.CENTER);
        
        Label takeLabel = Components.createText("Počet kamínků:");
        
        takeSpinner = new Spinner<>(1, gameState.getMaxTake(), 1);
        takeSpinner.setEditable(false);
        takeSpinner.setPrefWidth(80);
        takeSpinner.setStyle(Components.STYLE_TEXT_FIELD);
        
        takeButton = Components.createPrimaryButton("Vzít kamínky");
        takeButton.setOnAction(e -> handleTake());
        takeButton.setDisable(!gameState.isMyTurn());
        
        takeBox.getChildren().addAll(takeLabel, takeSpinner, takeButton);
        
        VBox skipBox = new VBox(10);
        skipBox.setAlignment(Pos.CENTER);
        
        Label skipLabel = Components.createText("Přeskočit tah:");
        
        skipButton = Components.createSecondaryButton("Přeskočit");
        skipButton.setOnAction(e -> handleSkip());
        skipButton.setDisable(!gameState.isMyTurn() || !gameState.canSkip());
        
        skipNoteLabel = Components.createTextLight(
                gameState.canSkip() ? "(zbývá 1 přeskočení)" : "(vyčerpáno)");
        
        skipBox.getChildren().addAll(skipLabel, skipButton, skipNoteLabel);
        
        controlsPanel.getChildren().addAll(takeBox, skipBox);

        // Game over overlay (skryty)
        gameOverPane = new VBox(20);
        gameOverPane.setAlignment(Pos.CENTER);
        gameOverPane.setStyle(String.format(
                "-fx-background-color: rgba(255,255,255,0.95); " +
                "-fx-background-radius: 15; " +
                "-fx-border-color: %s; " +
                "-fx-border-radius: 15; " +
                "-fx-border-width: 2;", Components.PRIMARY_COLOR));
        gameOverPane.setPadding(new Insets(40));
        gameOverPane.setMaxSize(400, 300);
        gameOverPane.setVisible(false);
        
        gameOverTitle = Components.createTitle("Konec hry!");
        gameOverMessage = Components.createHeading("");
        
        Button backToLobbyButton = Components.createPrimaryButton("Zpět do lobby");
        backToLobbyButton.setOnAction(e -> handleBackToLobby());
        
        gameOverPane.getChildren().addAll(gameOverTitle, gameOverMessage, backToLobbyButton);

        mainContent.getChildren().addAll(infoPanel, stonesCard, controlsPanel);

        // Stack pane pro overlay
        StackPane stackPane = new StackPane();
        stackPane.getChildren().addAll(mainContent, gameOverPane);
        StackPane.setAlignment(gameOverPane, Pos.CENTER);

        // Status bar
        HBox statusBar = Components.createStatusBar();
        statusIndicator = Components.createStatusIndicator(client.isConnected());
        statusLabel = Components.createTextLight(client.isConnected() ? "Připojeno" : "Odpojeno");
        statusBar.getChildren().addAll(statusIndicator, statusLabel);

        root.setTop(header);
        root.setCenter(stackPane);
        root.setBottom(statusBar);

        Scene scene = new Scene(root, 900, 700);
        stage.setScene(scene);

        Logger.info("Game view displayed");
    }

    /**
     * Vytvori vizualni reprezentaci kamenu.
     */
    private void createStoneCircles(int count) {
        stonesPane.getChildren().clear();
        stoneCircles.clear();
        
        for (int i = 0; i < count; i++) {
            Circle circle = new Circle(18);
            circle.setFill(Color.web(Components.PRIMARY_COLOR));
            circle.setStroke(Color.web(Components.SECONDARY_COLOR));
            circle.setStrokeWidth(2);
            
            // Efekt pri navratu mysi
            circle.setOnMouseEntered(e -> {
                circle.setFill(Color.web(Components.SECONDARY_COLOR));
            });
            circle.setOnMouseExited(e -> {
                circle.setFill(Color.web(Components.PRIMARY_COLOR));
            });
            
            stoneCircles.add(circle);
            stonesPane.getChildren().add(circle);
        }
    }

    /**
     * Odstrani kameny s animaci.
     */
    private void removeStonesAnimated(int count) {
        if (count <= 0 || stoneCircles.isEmpty()) return;
        
        int toRemove = Math.min(count, stoneCircles.size());
        
        for (int i = 0; i < toRemove; i++) {
            int index = stoneCircles.size() - 1 - i;
            if (index >= 0 && index < stoneCircles.size()) {
                Circle circle = stoneCircles.get(index);
                
                // Animace zmizeni
                FadeTransition fade = new FadeTransition(Duration.millis(300), circle);
                fade.setFromValue(1.0);
                fade.setToValue(0.0);
                
                ScaleTransition scale = new ScaleTransition(Duration.millis(300), circle);
                scale.setFromX(1.0);
                scale.setFromY(1.0);
                scale.setToX(0.0);
                scale.setToY(0.0);
                
                ParallelTransition parallel = new ParallelTransition(fade, scale);
                
                int finalIndex = index;
                parallel.setOnFinished(e -> {
                    stonesPane.getChildren().remove(circle);
                });
                
                parallel.play();
            }
        }
        
        // Odstran ze seznamu
        for (int i = 0; i < toRemove && !stoneCircles.isEmpty(); i++) {
            stoneCircles.remove(stoneCircles.size() - 1);
        }
    }

    /**
     * Aktualizuje UI podle stavu hry.
     */
    private void updateUI() {
        int stones = gameState.getStones();
        
        stonesCountLabel.setText(String.valueOf(stones));
        
        mySkipsLabel.setText("Vaše přeskočení: " + gameState.getMySkipsRemaining());
        oppSkipsLabel.setText("Soupeřova přeskočení: " + gameState.getOpponentSkipsRemaining());
        
        // Aktualizuj poznámku u tlačítka přeskočit
        if (skipNoteLabel != null) {
            skipNoteLabel.setText(gameState.canSkip() ? "(zbývá 1 přeskočení)" : "(vyčerpáno)");
        }
        
        // Aktualizuj spinner
        int maxTake = gameState.getMaxTake();
        SpinnerValueFactory.IntegerSpinnerValueFactory factory = 
                new SpinnerValueFactory.IntegerSpinnerValueFactory(1, Math.max(1, maxTake), 1);
        takeSpinner.setValueFactory(factory);
        
        // Aktualizuj stav protihrače a tlačítek
        updateOpponentStatus();
    }

    /**
     * Aktualizuje status protihrace.
     */
    private void updateOpponentStatus() {
        // Kontrola, zda jsou UI komponenty inicializovány
        if (opponentStatusLabel == null || takeButton == null || skipButton == null || turnLabel == null) {
            return;
        }
        
        GameState.OpponentStatus status = gameState.getOpponentStatus();
        
        switch (status) {
            case CONNECTED:
                opponentStatusLabel.setText("");
                opponentStatusLabel.setVisible(false);
                // Obnov tlačítka podle stavu hry
                updateButtonStates();
                break;
            case DISCONNECTED:
                opponentStatusLabel.setText("⚠ Soupeř je odpojen - hra pozastavena");
                opponentStatusLabel.setTextFill(Color.web(Components.WARNING_COLOR));
                opponentStatusLabel.setVisible(true);
                // Disabluj tlačítka - hra je pozastavena
                takeButton.setDisable(true);
                skipButton.setDisable(true);
                turnLabel.setText("Hra pozastavena");
                turnLabel.setTextFill(Color.web(Components.WARNING_COLOR));
                break;
            case RECONNECTED:
                opponentStatusLabel.setText("✓ Soupeř se znovu připojil");
                opponentStatusLabel.setTextFill(Color.web(Components.SUCCESS_COLOR));
                opponentStatusLabel.setVisible(true);
                // Obnov tlačítka podle stavu hry
                updateButtonStates();
                
                // Skryj po 3 sekundach
                Timeline timeline = new Timeline(new KeyFrame(Duration.seconds(3), e -> {
                    if (gameState.getOpponentStatus() == GameState.OpponentStatus.RECONNECTED) {
                        gameState.setOpponentStatus(GameState.OpponentStatus.CONNECTED);
                    }
                }));
                timeline.play();
                break;
        }
    }
    
    /**
     * Aktualizuje stav tlačítek podle herního stavu.
     */
    private void updateButtonStates() {
        // Kontrola, zda jsou UI komponenty inicializovány
        if (takeButton == null || skipButton == null || turnLabel == null) {
            return;
        }
        
        boolean myTurn = gameState.isMyTurn();
        boolean canPlay = myTurn && gameState.getStones() > 0 && 
                          gameState.getOpponentStatus() != GameState.OpponentStatus.DISCONNECTED;
        
        takeButton.setDisable(!canPlay);
        skipButton.setDisable(!canPlay || !gameState.canSkip());
        
        // Aktualizuj turn label
        if (gameState.getOpponentStatus() == GameState.OpponentStatus.DISCONNECTED) {
            turnLabel.setText("Hra pozastavena");
            turnLabel.setTextFill(Color.web(Components.WARNING_COLOR));
        } else {
            turnLabel.setText(myTurn ? "Jste na tahu!" : "Soupeř je na tahu");
            turnLabel.setTextFill(Color.web(myTurn ? Components.SUCCESS_COLOR : Components.TEXT_LIGHT));
        }
    }

    /**
     * Zobrazi game over obrazovku.
     */
    private void showGameOver() {
        String winner = gameState.getWinner();
        boolean iWon = winner.equals(gameState.getNickname());
        
        gameOverTitle.setText(iWon ? "Vyhráli jste!" : "Prohráli jste");
        gameOverTitle.setTextFill(Color.web(iWon ? Components.SUCCESS_COLOR : Components.DANGER_COLOR));
        
        gameOverMessage.setText(iWon ? 
                "Gratulujeme k výhře!" : 
                "Soupeř " + winner + " vyhrál");
        
        gameOverPane.setVisible(true);
        
        // Animace
        FadeTransition fade = new FadeTransition(Duration.millis(300), gameOverPane);
        fade.setFromValue(0);
        fade.setToValue(1);
        fade.play();
    }

    /**
     * Zpracuje tah - vzeti kamenu.
     */
    private void handleTake() {
        int count = takeSpinner.getValue();
        
        if (!gameState.isValidTake(count)) {
            Components.showError("Chyba", "Neplatný počet kamínků");
            return;
        }
        
        takeButton.setDisable(true);
        skipButton.setDisable(true);
        
        client.send(Protocol.createTake(count));
    }

    /**
     * Zpracuje preskoceni tahu.
     */
    private void handleSkip() {
        if (!gameState.canSkip()) {
            Components.showError("Chyba", "Nemáte žádné přeskočení");
            return;
        }
        
        takeButton.setDisable(true);
        skipButton.setDisable(true);
        
        client.send(Protocol.createSkip());
    }

    /**
     * Zpracuje opusteni hry.
     */
    private void handleLeaveGame() {
        if (gameState.getPhase() == GameState.Phase.IN_GAME) {
            boolean confirm = Components.showConfirm("Opustit hru?", 
                    "Opravdu chcete opustit hru? Prohra bude započítána.");
            if (!confirm) return;
        }
        
        client.send(Protocol.createLeaveRoom());
    }

    /**
     * Zpracuje navrat do lobby.
     */
    private void handleBackToLobby() {
        gameState.leaveRoom();
        LobbyView lobbyView = new LobbyView(stage, client, gameState);
        lobbyView.show();
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
                
                switch (state) {
                    case DISCONNECTED:
                        statusLabel.setText("Odpojeno");
                        // Disabluj ovládací prvky
                        if (takeButton != null) takeButton.setDisable(true);
                        if (skipButton != null) skipButton.setDisable(true);
                        break;
                    case CONNECTING:
                        statusLabel.setText("Připojování...");
                        break;
                    case CONNECTED:
                        statusLabel.setText("Připojeno");
                        // Obnov ovládací prvky podle stavu hry
                        updateUI();
                        break;
                    case RECONNECTING:
                        statusLabel.setText("Obnovování spojení...");
                        // Disabluj ovládací prvky během reconnectu
                        if (takeButton != null) takeButton.setDisable(true);
                        if (skipButton != null) skipButton.setDisable(true);
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
     * Nastavi listener na zmeny stavu hry.
     */
    private void setupStateListener() {
        gameState.setStateChangeListener(state -> {
            Platform.runLater(() -> {
                if (state.getPhase() == GameState.Phase.GAME_OVER) {
                    showGameOver();
                } else {
                    updateUI();
                }
            });
        });
    }

    /**
     * Zpracuje prijatou zpravu.
     */
    private void handleMessage(Protocol.ParsedMessage message) {
        Logger.debug("Game handling message: %s", message.getType());
        
        switch (message.getType()) {
            case TAKE_OK:
                handleTakeOk(message);
                break;
                
            case TAKE_ERR:
                handleTakeErr(message);
                break;
                
            case SKIP_OK:
                handleSkipOk(message);
                break;
                
            case SKIP_ERR:
                handleSkipErr(message);
                break;
                
            case OPPONENT_ACTION:
                handleOpponentAction(message);
                break;
                
            case GAME_OVER:
                handleGameOver(message);
                break;
                
            case PLAYER_STATUS:
                handlePlayerStatus(message);
                break;
                
            case LEAVE_OK:
                gameState.leaveRoom();
                LobbyView lobbyViewLeave = new LobbyView(stage, client, gameState);
                lobbyViewLeave.show();
                break;
                
            case GAME_RESUMED:
                handleGameResumed(message);
                break;
            
            case LOGIN_OK:
                // Server restartoval - nemá náš herní stav
                // Přejdi do lobby (bez GAME_RESUMED = žádná hra k obnovení)
                Logger.info("Server restarted, returning to lobby");
                gameState.reset();
                LobbyView lobbyViewReset = new LobbyView(stage, client, gameState);
                lobbyViewReset.show();
                break;
                
            case LOGIN_ERR:
                // Chyba při reconnectu - vrať na login
                Logger.warning("Login failed after reconnect: %s", message.getParam(1));
                gameState.reset();
                LoginView loginView = new LoginView(stage);
                loginView.show();
                break;
                
            case ERROR:
                Components.showError("Chyba", "Chyba serveru: " + message.getParam(1));
                break;
                
            default:
                Logger.warning("Unexpected message in game: %s", message.getType());
                break;
        }
    }

    /**
     * Zpracuje uspesne vzeti kamenu.
     */
    private void handleTakeOk(Protocol.ParsedMessage message) {
        int remaining = message.getParamAsInt(0);
        boolean stillMyTurn = message.getParamAsBoolean(1);
        
        int taken = gameState.getStones() - remaining;
        removeStonesAnimated(taken);
        
        gameState.myTakeSucceeded(remaining, stillMyTurn);
    }

    /**
     * Zpracuje chybu pri brani.
     */
    private void handleTakeErr(Protocol.ParsedMessage message) {
        int errorCode = message.getParamAsInt(0);
        Protocol.ErrorCode code = Protocol.ErrorCode.fromCode(errorCode);
        Components.showError("Chyba tahu", Protocol.getErrorMessage(code));
        
        updateUI();
    }

    /**
     * Zpracuje uspesne preskoceni.
     */
    private void handleSkipOk(Protocol.ParsedMessage message) {
        boolean stillMyTurn = message.getParamAsBoolean(0);
        gameState.mySkipSucceeded(stillMyTurn);
    }

    /**
     * Zpracuje chybu pri preskoceni.
     */
    private void handleSkipErr(Protocol.ParsedMessage message) {
        int errorCode = message.getParamAsInt(0);
        Protocol.ErrorCode code = Protocol.ErrorCode.fromCode(errorCode);
        Components.showError("Chyba", Protocol.getErrorMessage(code));
        
        updateUI();
    }

    /**
     * Zpracuje akci protihrace.
     */
    private void handleOpponentAction(Protocol.ParsedMessage message) {
        String action = message.getParam(0);
        int param = message.getParamAsInt(1);
        int remaining = message.getParamAsInt(2);
        
        if ("TAKE".equals(action)) {
            int taken = gameState.getStones() - remaining;
            removeStonesAnimated(taken);
            gameState.opponentTook(param, remaining);
        } else if ("SKIP".equals(action)) {
            gameState.opponentSkipped(remaining);
        }
    }

    /**
     * Zpracuje konec hry.
     */
    private void handleGameOver(Protocol.ParsedMessage message) {
        String winner = message.getParam(0);
        String loser = message.getParam(1);
        
        gameState.endGame(winner, loser);
    }

    /**
     * Zpracuje zmenu stavu hrace.
     */
    private void handlePlayerStatus(Protocol.ParsedMessage message) {
        String nickname = message.getParam(0);
        String status = message.getParam(1);
        
        if (!nickname.equals(gameState.getNickname())) {
            // Je to protihrac
            switch (status) {
                case "DISCONNECTED":
                    gameState.setOpponentStatus(GameState.OpponentStatus.DISCONNECTED);
                    break;
                case "RECONNECTED":
                    gameState.setOpponentStatus(GameState.OpponentStatus.RECONNECTED);
                    break;
                case "CONNECTED":
                    gameState.setOpponentStatus(GameState.OpponentStatus.CONNECTED);
                    break;
            }
        }
    }

    /**
     * Zpracuje obnoveni hry.
     */
    private void handleGameResumed(Protocol.ParsedMessage message) {
        int stones = message.getParamAsInt(0);
        boolean myTurn = message.getParamAsBoolean(1);
        int mySkips = message.getParamAsInt(2);
        int oppSkips = message.getParamAsInt(3);
        
        gameState.resumeGame(stones, myTurn, mySkips, oppSkips);
        createStoneCircles(stones);
        updateUI();
    }
}

