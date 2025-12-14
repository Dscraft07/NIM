package nim;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.stage.Stage;
import nim.ui.LoginView;
import nim.util.Logger;

/**
 * Hlavni trida aplikace - vstupni bod JavaFX klienta.
 * 
 * @author Student
 * @version 1.0
 */
public class Main extends Application {

    private static final String APP_TITLE = "Nim Game";

    @Override
    public void start(Stage primaryStage) {
        Logger.info("Application starting...");

        primaryStage.setTitle(APP_TITLE);
        primaryStage.setMinWidth(800);
        primaryStage.setMinHeight(600);
        primaryStage.setResizable(true);

        // Pri zavreni okna ukonci aplikaci
        primaryStage.setOnCloseRequest(event -> {
            Logger.info("Application closing...");
            Platform.exit();
            System.exit(0);
        });

        // Zobraz login obrazovku
        LoginView loginView = new LoginView(primaryStage);
        loginView.show();

        primaryStage.show();
        Logger.info("Application started successfully");
    }

    /**
     * Vstupni bod aplikace.
     * 
     * @param args argumenty prikazove radky
     */
    public static void main(String[] args) {
        launch(args);
    }
}

