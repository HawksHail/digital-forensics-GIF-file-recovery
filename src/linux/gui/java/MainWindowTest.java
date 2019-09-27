/*
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.fest.assertions.Assertions.assertThat;

import org.mockito.runners.MockitoJUnitRunner;

import javax.swing.*;
import java.awt.*;

import static org.junit.Assert.*;


@RunWith(MockitoJUnitRunner.class)
public class MainWindowTest {
    private MainWindow underTest;
    private static String VALIDATE = "Validate";
    private static String UPONELEVEL = "Up one level";


    @Before
    public void setUp() throws Exception {
        underTest = new MainWindow();
        underTest.createAndShowGUI();

    }

    @After
    public void tearDown() throws Exception {

    }

    @Test
    public void main() throws Exception {

        assertNotNull(underTest);
        assertThat(underTest.getContent()).isInstanceOf(JPanel.class);
        assertTrue(underTest.getContent().isEnabled());
        assertTrue(underTest.getContent().isVisible());
        assertThat(underTest.getContent().getComponentCount()).isGreaterThanOrEqualTo(4);
        assertThat(underTest.getContent().getComponent(0)).isInstanceOf(Choice.class);
        assertThat(underTest.getContent().getComponent(1)).isInstanceOf(JScrollPane.class);
        assertThat(underTest.getContent().getComponent(2)).isInstanceOf(JPanel.class);
        assertThat(underTest.getContent().getComponent(3)).isInstanceOf(JPanel.class);
        assertThat(underTest.getgCurrentLevel()).isGreaterThanOrEqualTo(1);
    }

    @Test
    public void getDiskInformation() throws Exception {
        assertThat(underTest.getDiskInformation()).isNotEmpty();
        assertThat(underTest.getCurrentDisk()).isEqualTo("/dev/sda");
    }

*/
/*    // TODO Failing maybe because of JNI library dependencies not mapped, or some logic error with code

@Test
    public void createBootBlock() throws Exception {
        underTest.createBootBlock(new JFXPanel());

    }*//*




    @Test
    public void getgDetailsText() throws Exception {
        assertNotNull(underTest.getgDetailsText());
        assertThat(underTest.getgDetailsText()).isInstanceOf(JTextPane.class);
        assertTrue(underTest.getgDetailsText().isEnabled());
        assertTrue(underTest.getgDetailsText().isVisible());
        assertTrue(underTest.getgDetailsText().isValid());
        assertThat(underTest.getgDetailsText().getParent()).isInstanceOf(JPanel.class);

    }

    @Test
    public void getgButtonZoomOut() throws Exception {
        assertNotNull(underTest.getgButtonZoomOut());
        assertThat(underTest.getgButtonZoomOut().getText()).isEqualTo(UPONELEVEL);
        assertThat(underTest.getgButtonZoomOut().getModel()).isInstanceOf(DefaultButtonModel.class);
        assertFalse(underTest.getgButtonZoomOut().isEnabled());
        assertTrue(underTest.getgButtonZoomOut().isVisible());
        assertTrue(underTest.getgButtonZoomOut().isValid());

    }

    @Test
    public void getgButtonValidate() throws Exception {
        assertNotNull(underTest.getgButtonValidate());
        assertThat(underTest.getgButtonValidate().getText()).isEqualTo(VALIDATE);
        assertThat(underTest.getgButtonValidate().getModel()).isInstanceOf(DefaultButtonModel.class);
        assertFalse(underTest.getgButtonValidate().isEnabled());
        assertTrue(underTest.getgButtonValidate().isVisible());
        assertTrue(underTest.getgButtonValidate().isValid());

    }

    @Test
    public void getColorCode() throws Exception {
        assertNotNull(underTest.getColorCode());
        assertThat(underTest.getColorCode()).hasSize(7);
    }


    @Test
    public void getgProgressBar() throws Exception {
        assertNotNull(underTest.getgProgressBar());
        assertThat(underTest.getgProgressBar()).isInstanceOf(JProgressBar.class);
        assertTrue(underTest.getgProgressBar().isEnabled());
        assertTrue(underTest.getgProgressBar().isValid());

    }

}

*/
