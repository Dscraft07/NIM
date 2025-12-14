package nim.ui;

import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;

/**
 * Sdilene UI komponenty a styly.
 * Pouziva jemnou, prijemnou barevnou paletu.
 */
public class Components {

    // ============================================
    // BARVY - jemna paleta
    // ============================================
    
    /** Pozadi aplikace */
    public static final String BG_COLOR = "#f5f5f0";
    
    /** Hlavni akcent */
    public static final String PRIMARY_COLOR = "#5c7a6f";
    
    /** Sekundarni akcent */
    public static final String SECONDARY_COLOR = "#8fa89a";
    
    /** Cervena pro chyby/prohru */
    public static final String DANGER_COLOR = "#c17767";
    
    /** Zelena pro uspech/vyhru */
    public static final String SUCCESS_COLOR = "#7a9e7e";
    
    /** Zluta pro varovani */
    public static final String WARNING_COLOR = "#d4a76a";
    
    /** Text */
    public static final String TEXT_COLOR = "#3d3d3d";
    
    /** Svetly text */
    public static final String TEXT_LIGHT = "#6b6b6b";
    
    /** Pozadi karty */
    public static final String CARD_BG = "#ffffff";
    
    /** Ohraniceni */
    public static final String BORDER_COLOR = "#d4d4d0";

    // ============================================
    // FONTY
    // ============================================
    
    public static final Font FONT_TITLE = Font.font("Georgia", FontWeight.BOLD, 32);
    public static final Font FONT_HEADING = Font.font("Georgia", FontWeight.BOLD, 24);
    public static final Font FONT_SUBHEADING = Font.font("Georgia", FontWeight.NORMAL, 18);
    public static final Font FONT_BODY = Font.font("Segoe UI", FontWeight.NORMAL, 14);
    public static final Font FONT_SMALL = Font.font("Segoe UI", FontWeight.NORMAL, 12);
    public static final Font FONT_MONO = Font.font("Consolas", FontWeight.NORMAL, 14);

    // ============================================
    // STYLY
    // ============================================
    
    public static final String STYLE_BASE = String.format(
            "-fx-font-family: 'Segoe UI'; " +
            "-fx-font-size: 14px; " +
            "-fx-text-fill: %s;", TEXT_COLOR);
    
    public static final String STYLE_BUTTON_PRIMARY = String.format(
            "-fx-background-color: %s; " +
            "-fx-text-fill: white; " +
            "-fx-font-size: 14px; " +
            "-fx-font-weight: bold; " +
            "-fx-padding: 10 20; " +
            "-fx-background-radius: 5; " +
            "-fx-cursor: hand;", PRIMARY_COLOR);
    
    public static final String STYLE_BUTTON_PRIMARY_HOVER = String.format(
            "-fx-background-color: derive(%s, -10%%); " +
            "-fx-text-fill: white; " +
            "-fx-font-size: 14px; " +
            "-fx-font-weight: bold; " +
            "-fx-padding: 10 20; " +
            "-fx-background-radius: 5; " +
            "-fx-cursor: hand;", PRIMARY_COLOR);
    
    public static final String STYLE_BUTTON_SECONDARY = String.format(
            "-fx-background-color: transparent; " +
            "-fx-text-fill: %s; " +
            "-fx-font-size: 14px; " +
            "-fx-padding: 10 20; " +
            "-fx-background-radius: 5; " +
            "-fx-border-color: %s; " +
            "-fx-border-radius: 5; " +
            "-fx-border-width: 1; " +
            "-fx-cursor: hand;", PRIMARY_COLOR, PRIMARY_COLOR);
    
    public static final String STYLE_BUTTON_DANGER = String.format(
            "-fx-background-color: %s; " +
            "-fx-text-fill: white; " +
            "-fx-font-size: 14px; " +
            "-fx-font-weight: bold; " +
            "-fx-padding: 10 20; " +
            "-fx-background-radius: 5; " +
            "-fx-cursor: hand;", DANGER_COLOR);
    
    public static final String STYLE_TEXT_FIELD = String.format(
            "-fx-background-color: white; " +
            "-fx-border-color: %s; " +
            "-fx-border-radius: 5; " +
            "-fx-background-radius: 5; " +
            "-fx-padding: 10; " +
            "-fx-font-size: 14px;", BORDER_COLOR);
    
    public static final String STYLE_CARD = String.format(
            "-fx-background-color: %s; " +
            "-fx-background-radius: 10; " +
            "-fx-border-color: %s; " +
            "-fx-border-radius: 10; " +
            "-fx-border-width: 1; " +
            "-fx-effect: dropshadow(gaussian, rgba(0,0,0,0.1), 10, 0, 0, 2);", 
            CARD_BG, BORDER_COLOR);

    // ============================================
    // KOMPONENTY
    // ============================================
    
    /**
     * Vytvori stylizovany nadpis.
     */
    public static Label createTitle(String text) {
        Label label = new Label(text);
        label.setFont(FONT_TITLE);
        label.setTextFill(Color.web(TEXT_COLOR));
        return label;
    }
    
    /**
     * Vytvori stylizovany podnadpis.
     */
    public static Label createHeading(String text) {
        Label label = new Label(text);
        label.setFont(FONT_HEADING);
        label.setTextFill(Color.web(TEXT_COLOR));
        return label;
    }
    
    /**
     * Vytvori stylizovany text.
     */
    public static Label createText(String text) {
        Label label = new Label(text);
        label.setFont(FONT_BODY);
        label.setTextFill(Color.web(TEXT_COLOR));
        return label;
    }
    
    /**
     * Vytvori svetly text.
     */
    public static Label createTextLight(String text) {
        Label label = new Label(text);
        label.setFont(FONT_BODY);
        label.setTextFill(Color.web(TEXT_LIGHT));
        return label;
    }
    
    /**
     * Vytvori primarni tlacitko.
     */
    public static Button createPrimaryButton(String text) {
        Button button = new Button(text);
        button.setStyle(STYLE_BUTTON_PRIMARY);
        button.setOnMouseEntered(e -> button.setStyle(STYLE_BUTTON_PRIMARY_HOVER));
        button.setOnMouseExited(e -> button.setStyle(STYLE_BUTTON_PRIMARY));
        return button;
    }
    
    /**
     * Vytvori sekundarni tlacitko.
     */
    public static Button createSecondaryButton(String text) {
        Button button = new Button(text);
        button.setStyle(STYLE_BUTTON_SECONDARY);
        return button;
    }
    
    /**
     * Vytvori nebezpecne tlacitko (cervene).
     */
    public static Button createDangerButton(String text) {
        Button button = new Button(text);
        button.setStyle(STYLE_BUTTON_DANGER);
        return button;
    }
    
    /**
     * Vytvori stylizovany text field.
     */
    public static TextField createTextField(String prompt) {
        TextField field = new TextField();
        field.setPromptText(prompt);
        field.setStyle(STYLE_TEXT_FIELD);
        field.setPrefWidth(250);
        return field;
    }
    
    /**
     * Vytvori kartu (panel s pozadim a stinem).
     */
    public static VBox createCard() {
        VBox card = new VBox(15);
        card.setStyle(STYLE_CARD);
        card.setPadding(new Insets(25));
        card.setAlignment(Pos.CENTER);
        return card;
    }
    
    /**
     * Vytvori horizontalni oddelovac.
     */
    public static Region createSpacer() {
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        return spacer;
    }
    
    /**
     * Vytvori vertikalni oddelovac.
     */
    public static Region createVSpacer() {
        Region spacer = new Region();
        VBox.setVgrow(spacer, Priority.ALWAYS);
        return spacer;
    }
    
    /**
     * Vytvori chybovou zpravu.
     */
    public static Label createErrorLabel(String text) {
        Label label = new Label(text);
        label.setFont(FONT_BODY);
        label.setTextFill(Color.web(DANGER_COLOR));
        label.setWrapText(true);
        return label;
    }
    
    /**
     * Vytvori uspesnou zpravu.
     */
    public static Label createSuccessLabel(String text) {
        Label label = new Label(text);
        label.setFont(FONT_BODY);
        label.setTextFill(Color.web(SUCCESS_COLOR));
        label.setWrapText(true);
        return label;
    }
    
    /**
     * Vytvori varovnou zpravu.
     */
    public static Label createWarningLabel(String text) {
        Label label = new Label(text);
        label.setFont(FONT_BODY);
        label.setTextFill(Color.web(WARNING_COLOR));
        label.setWrapText(true);
        return label;
    }
    
    /**
     * Vytvori status bar.
     */
    public static HBox createStatusBar() {
        HBox statusBar = new HBox(10);
        statusBar.setAlignment(Pos.CENTER_LEFT);
        statusBar.setPadding(new Insets(10, 15, 10, 15));
        statusBar.setStyle(String.format(
                "-fx-background-color: %s; " +
                "-fx-border-color: %s; " +
                "-fx-border-width: 1 0 0 0;", CARD_BG, BORDER_COLOR));
        return statusBar;
    }
    
    /**
     * Vytvori indikator stavu (kruzek).
     */
    public static Region createStatusIndicator(boolean connected) {
        Region indicator = new Region();
        indicator.setMinSize(10, 10);
        indicator.setMaxSize(10, 10);
        String color = connected ? SUCCESS_COLOR : DANGER_COLOR;
        indicator.setStyle(String.format(
                "-fx-background-color: %s; " +
                "-fx-background-radius: 5;", color));
        return indicator;
    }
    
    /**
     * Vytvori hlavni layout stranky.
     */
    public static BorderPane createPageLayout() {
        BorderPane layout = new BorderPane();
        layout.setStyle(String.format("-fx-background-color: %s;", BG_COLOR));
        layout.setPadding(new Insets(20));
        return layout;
    }
    
    /**
     * Zobrazi informacni dialog.
     */
    public static void showInfo(String title, String message) {
        Alert alert = new Alert(Alert.AlertType.INFORMATION);
        alert.setTitle(title);
        alert.setHeaderText(null);
        alert.setContentText(message);
        alert.showAndWait();
    }
    
    /**
     * Zobrazi chybovy dialog.
     */
    public static void showError(String title, String message) {
        Alert alert = new Alert(Alert.AlertType.ERROR);
        alert.setTitle(title);
        alert.setHeaderText(null);
        alert.setContentText(message);
        alert.showAndWait();
    }
    
    /**
     * Zobrazi potvrzovaci dialog.
     */
    public static boolean showConfirm(String title, String message) {
        Alert alert = new Alert(Alert.AlertType.CONFIRMATION);
        alert.setTitle(title);
        alert.setHeaderText(null);
        alert.setContentText(message);
        return alert.showAndWait().orElse(ButtonType.CANCEL) == ButtonType.OK;
    }
}

