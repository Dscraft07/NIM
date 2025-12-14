package nim.ui;

import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.Stage;
import nim.game.GameState;
import nim.network.Client;
import nim.network.Protocol;
import nim.util.Logger;

/**
 * Prihlasovaci obrazovka.
 * Umoznuje zadani IP adresy, portu a prezdivky.
 */
public class LoginView {

    private final Stage stage;
    private final GameState gameState;
    private final Client client;

    // UI komponenty
    private TextField hostField;
    private TextField portField;
    private TextField nicknameField;
    private Button connectButton;
    private Label statusLabel;
    private Label errorLabel;
    private ProgressIndicator progressIndicator;

    public LoginView(Stage stage) {
        this.stage = stage;
        this.gameState = new GameState();
        this.client = new Client();
        
        setupClientHandlers();
    }

    /**
     * Zobrazi prihlasovaci obrazovku.
     */
    public void show() {
        BorderPane root = Components.createPageLayout();
        
        // Hlavni obsah
        VBox content = new VBox(30);
        content.setAlignment(Pos.CENTER);
        content.setMaxWidth(400);
        
        // Logo/nadpis
        VBox header = new VBox(10);
        header.setAlignment(Pos.CENTER);
        Label title = Components.createTitle("NIM");
        Label subtitle = Components.createTextLight("Síťová hra pro dva hráče");
        header.getChildren().addAll(title, subtitle);
        
        // Formular
        VBox form = Components.createCard();
        form.setMaxWidth(350);
        
        Label formTitle = Components.createHeading("Připojení");
        formTitle.setPadding(new Insets(0, 0, 10, 0));
        
        // Host
        VBox hostGroup = createFormGroup("Server", "127.0.0.1");
        hostField = (TextField) hostGroup.getChildren().get(1);
        hostField.setText("127.0.0.1");
        
        // Port
        VBox portGroup = createFormGroup("Port", "10000");
        portField = (TextField) portGroup.getChildren().get(1);
        portField.setText("10000");
        
        // Prezdivka
        VBox nicknameGroup = createFormGroup("Přezdívka", "Hrac1");
        nicknameField = (TextField) nicknameGroup.getChildren().get(1);
        
        // Tlacitko
        connectButton = Components.createPrimaryButton("Připojit");
        connectButton.setMaxWidth(Double.MAX_VALUE);
        connectButton.setOnAction(e -> handleConnect());
        
        // Progress indicator
        progressIndicator = new ProgressIndicator();
        progressIndicator.setMaxSize(30, 30);
        progressIndicator.setVisible(false);
        
        // Chybova zprava
        errorLabel = Components.createErrorLabel("");
        errorLabel.setVisible(false);
        
        form.getChildren().addAll(
                formTitle,
                hostGroup,
                portGroup,
                nicknameGroup,
                new Region() {{ setMinHeight(10); }},
                connectButton,
                progressIndicator,
                errorLabel
        );
        
        content.getChildren().addAll(header, form);
        
        // Status bar
        HBox statusBar = Components.createStatusBar();
        statusLabel = Components.createTextLight("Odpojeno");
        Region indicator = Components.createStatusIndicator(false);
        statusBar.getChildren().addAll(indicator, statusLabel);
        
        root.setCenter(content);
        root.setBottom(statusBar);
        
        Scene scene = new Scene(root, 800, 600);
        stage.setScene(scene);
        
        // Focus na prezdivku
        Platform.runLater(() -> nicknameField.requestFocus());
        
        // Enter = pripojit
        nicknameField.setOnAction(e -> handleConnect());
        
        Logger.info("Login view displayed");
    }

    /**
     * Vytvori skupinu formulare (label + textfield).
     */
    private VBox createFormGroup(String label, String placeholder) {
        VBox group = new VBox(5);
        Label lbl = Components.createText(label);
        TextField field = Components.createTextField(placeholder);
        group.getChildren().addAll(lbl, field);
        return group;
    }

    /**
     * Zpracuje pripojeni.
     */
    private void handleConnect() {
        String host = hostField.getText().trim();
        String portStr = portField.getText().trim();
        String nickname = nicknameField.getText().trim();
        
        // Validace
        if (host.isEmpty()) {
            showError("Zadejte adresu serveru");
            hostField.requestFocus();
            return;
        }
        
        int port;
        try {
            port = Integer.parseInt(portStr);
            if (port <= 0 || port > 65535) {
                throw new NumberFormatException();
            }
        } catch (NumberFormatException e) {
            showError("Neplatný port (1-65535)");
            portField.requestFocus();
            return;
        }
        
        if (nickname.isEmpty()) {
            showError("Zadejte přezdívku");
            nicknameField.requestFocus();
            return;
        }
        
        if (!nickname.matches("^[a-zA-Z][a-zA-Z0-9_]{0,31}$")) {
            showError("Přezdívka musí začínat písmenem a obsahovat pouze písmena, čísla a podtržítka (max 32 znaků)");
            nicknameField.requestFocus();
            return;
        }
        
        // Pripojeni
        setConnecting(true);
        hideError();
        
        gameState.setNickname(nickname);
        gameState.setPhase(GameState.Phase.CONNECTING);
        
        // Spust ve vlastnim vlakne
        new Thread(() -> {
            boolean success = client.connect(host, port, nickname);
            
            Platform.runLater(() -> {
                if (!success) {
                    setConnecting(false);
                    showError("Nepodařilo se připojit k serveru");
                    gameState.setPhase(GameState.Phase.DISCONNECTED);
                }
                // Uspech se zpracuje v message handleru
            });
        }).start();
    }

    /**
     * Nastavi stav pripojovani.
     */
    private void setConnecting(boolean connecting) {
        connectButton.setDisable(connecting);
        hostField.setDisable(connecting);
        portField.setDisable(connecting);
        nicknameField.setDisable(connecting);
        progressIndicator.setVisible(connecting);
        
        if (connecting) {
            statusLabel.setText("Připojování...");
        }
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
     * Nastavi handlery pro klienta.
     */
    private void setupClientHandlers() {
        client.setMessageHandler(message -> {
            Platform.runLater(() -> handleMessage(message));
        });
        
        client.setConnectionStateHandler(state -> {
            Platform.runLater(() -> {
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
                if (gameState.getPhase() != GameState.Phase.DISCONNECTED &&
                    gameState.getPhase() != GameState.Phase.CONNECTING &&
                    gameState.getPhase() != GameState.Phase.LOGIN) {
                    Components.showError("Odpojeno", "Spojení se serverem bylo ztraceno.");
                    show(); // Zobraz login
                }
            });
        });
    }

    /**
     * Zpracuje prijatou zpravu.
     */
    private void handleMessage(Protocol.ParsedMessage message) {
        Logger.debug("Login handling message: %s", message.getType());
        
        switch (message.getType()) {
            case LOGIN_OK:
                Logger.info("Login successful");
                gameState.setPhase(GameState.Phase.LOBBY);
                // Prejdi do lobby
                LobbyView lobbyView = new LobbyView(stage, client, gameState);
                lobbyView.show();
                break;
                
            case GAME_RESUMED:
                // Reconnect do rozehrane hry - jdi primo do GameView
                Logger.info("Reconnected to running game");
                handleGameResumed(message);
                break;
                
            case LOGIN_ERR:
                setConnecting(false);
                int errorCode = message.getParamAsInt(0);
                String errorMsg = message.getParam(1);
                Protocol.ErrorCode code = Protocol.ErrorCode.fromCode(errorCode);
                showError(Protocol.getErrorMessage(code) + 
                          (errorMsg.isEmpty() ? "" : ": " + errorMsg));
                gameState.setPhase(GameState.Phase.DISCONNECTED);
                client.disconnect();
                break;
                
            case ERROR:
                setConnecting(false);
                showError("Chyba serveru: " + message.getParam(1));
                break;
                
            default:
                Logger.warning("Unexpected message in login: %s", message.getType());
                break;
        }
    }
    
    /**
     * Zpracuje obnoveni hry (po reconnectu).
     * Server posle GAME_RESUMED ihned po LOGIN_OK pri reconnectu.
     */
    private void handleGameResumed(Protocol.ParsedMessage message) {
        int stones = message.getParamAsInt(0);
        boolean myTurn = message.getParamAsBoolean(1);
        int mySkips = message.getParamAsInt(2);
        int oppSkips = message.getParamAsInt(3);
        
        gameState.resumeGame(stones, myTurn, mySkips, oppSkips);
        
        // Prejdi primo na herni obrazovku
        GameView gameView = new GameView(stage, client, gameState);
        gameView.show();
    }
}

