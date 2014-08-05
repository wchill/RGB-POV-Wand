import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.Scanner;
import java.awt.Color;
import javax.imageio.ImageIO;
import javax.swing.*;
 
public class ImageConverter {
        public static void main(String[] args) {
                File file = null;
                if(args.length < 1) {
                        final JFileChooser fc = new JFileChooser();
                        fc.setDialogTitle("Choose an image");
                        int returnVal = fc.showOpenDialog(null);
                        if (returnVal == JFileChooser.APPROVE_OPTION) {
                            file = fc.getSelectedFile();
                        } else {
                            System.exit(0);
                        }
                } else {
                        file = new File(args[0]);
                }
                BufferedImage img;
                try {
                        img=ImageIO.read(file);
                        if(img.getWidth() == 0 || img.getWidth() > 127 || img.getHeight() != 8) {
                                JOptionPane.showMessageDialog(new JFrame(), "Image size must be between 1 and 127 pixels wide inclusive and 8 pixels tall", "Image size out of bounds", JOptionPane.ERROR_MESSAGE);
                                System.exit(1);
                        }
                        char[] data = new char[img.getWidth() * img.getHeight() + 2];
                        data[0] = (char) ((img.getWidth() & 0xFF) >> 8);
                        data[1] = (char) (img.getWidth() & 0xFF);
                        for(int i = 0; i < img.getWidth(); i++) {
                                for(int j = 0; j < 8; j++) {
                                        Color c = new Color(img.getRGB(i,j));
                                        int red = c.getRed();
                                        int green = c.getGreen();
                                        int blue = c.getBlue();
                                        char r = (char) (red & 0xE0);
                                        char g = (char) ((green >> 3) & 0x1C);
                                        char b = (char) ((blue >> 6) & 0x03);
                                        char ch = (char) (r | g | b);
                                        data[8 * i + j + 2] = ch;
                                }
                        }
                        StringBuffer sb = new StringBuffer();
                        sb.append("PROGMEM prog_uchar image[] = {" + (int) data[0] + "," + (int) data[1] + ",\n");
                        int count = 0;
                        for(int i = 2; i < (img.getWidth() * 8) - 1; i++) {
                                sb.append((int) data[i]);
                                sb.append(",");
                                if(++count % 32 == 0) sb.append("\n");
                        }
                        sb.append((int) data[data.length - 1]);
                        sb.append("\n};\n");
                        JTextArea textarea= new JTextArea(sb.toString());
                        textarea.setEditable(true);
                        JOptionPane.showMessageDialog(null, textarea, "Copy paste this code into Arduino", JOptionPane.INFORMATION_MESSAGE);
                } catch(IOException e) {
                        e.printStackTrace();
                }
        }
}


