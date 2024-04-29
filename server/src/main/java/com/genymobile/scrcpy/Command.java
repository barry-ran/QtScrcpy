package com.genymobile.scrcpy;

import io.github.pixee.security.SystemCommand;
import java.io.IOException;
import java.util.Arrays;
import java.util.Scanner;

public final class Command {
    private Command() {
        // not instantiable
    }

    public static void exec(String... cmd) throws IOException, InterruptedException {
        Process process = SystemCommand.runCommand(Runtime.getRuntime(), cmd);
        int exitCode = process.waitFor();
        if (exitCode != 0) {
            throw new IOException("Command " + Arrays.toString(cmd) + " returned with value " + exitCode);
        }
    }

    public static String execReadLine(String... cmd) throws IOException, InterruptedException {
        String result = null;
        Process process = SystemCommand.runCommand(Runtime.getRuntime(), cmd);
        Scanner scanner = new Scanner(process.getInputStream());
        if (scanner.hasNextLine()) {
            result = scanner.nextLine();
        }
        int exitCode = process.waitFor();
        if (exitCode != 0) {
            throw new IOException("Command " + Arrays.toString(cmd) + " returned with value " + exitCode);
        }
        return result;
    }
}
