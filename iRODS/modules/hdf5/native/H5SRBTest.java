import java.io.*;
import ncsa.hdf.srb.*;

/**
**/
public class H5SRBTest {
    /*
        IRODSAccount:
            String host,                    // accountInfo[0]
            int port,                       // accountInfo[1]
            String userName,                // accountInfo[2]
            String password,                // accountInfo[3]
            String homeDirectory,           // accountInfo[4]
            String mdasDomainNamee/Zone,    // accountInfo[5]
            String defaultStorageResource   // accountInfo[6]
    */
    private final String accountInfo[] = new String[7];
    private H5SrbFile srbFile;
    private int       fid;

    public H5SRBTest(String filename) {
        accountInfo[0] = "kagiso.hdfgroup.uiuc.edu";
        accountInfo[1] = "1247";
        accountInfo[2] = "rods";
        accountInfo[3] = "hdf5irodsdemo2008";
        accountInfo[4] = "/tempZone/home/rods";
        accountInfo[5] = "tempZone";
        accountInfo[6] = "demoResc";
        
       srbFile = new H5SrbFile(filename);

        try {
            int fid = srbFile.open();
        } catch (Throwable err) {
            err.printStackTrace();
        }
    }

    public static void main(final String[] args) {
        if (args.length < 1) {
            System.out.println("\nH5SRBTest filename\n");
            System.exit(0);
         }

         H5SRBTest test = new H5SRBTest(args[0]);
    }

}


