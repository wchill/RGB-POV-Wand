import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import javax.imageio.ImageIO;
 
public class SimpleConvertImage {
        public static void main(String[] args) throws IOException{
                if(args.length < 1) throw new IOException("No file provided");
                BufferedImage img=ImageIO.read(new File(args[0]));
                if(img.getWidth() == 0 || img.getWidth() > 127 || img.getHeight() != 8) {
                        System.err.println("Image size must be between 1 and 127 pixels wide inclusive and 8 pixels tall");
                } else {
                        char[] data = new char[img.getWidth() * img.getHeight() + 2];
                        data[0] = (char) ((img.getWidth() & 0xFF) >> 8);
                        data[1] = (char) (img.getWidth() & 0xFF);
                        for(int i = 0; i < img.getWidth(); i++) {
                                for(int j = 0; j < 8; j++) {
                                        int px = img.getRGB(i, j);
                                        int red = px >> 16;
                                        int green = (px >> 8) & 0xFF;
                                        int blue = px & 0xFF;
                                        char r = (char) (red & 0xE0);
                                        char g = (char) ((green >> 3) & 0x1C);
                                        char b = (char) ((blue >> 6) & 0x03);
                                        char c = (char) (r | g | b);
                                        data[8 * i + j + 2] = c;
                                }
                        }
                        System.out.println("PROGMEM prog_uchar image[] = {" + (int) data[0] + "," + (int) data[1] + ",");
                        for(int i = 2; i < img.getWidth() - 1; i++) {
                                System.out.print((int) data[i]);
                                System.out.print(",");
                        }
                        System.out.print((int) data[data.length - 1]);
                        System.out.println("\n};");
                }
        }
}
