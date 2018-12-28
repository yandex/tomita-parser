package com.srg.tomita;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Collection;
import java.util.Objects;

/**
 * Java-биндинг для Yandex Tomita Parser (https://github.com/yandex/tomita-parser).
 * <p>
 * Необходимые динамические библиотеки должны лежать в системных путях (LD_LIBRARY_PATH в unix-like системах).
 * Это как минимум две библиотеки: libFactExtract-Parser-textminerlib_java и libmystem_c_binding.
 *
 * Внимание! Этот класс не потокобезопасен (будут гонки в нативном коде). Рекомендуется создавать один
 * инстанс данного класса на поток. Для многопоточного использование есть класс {@link TomitaPooledParser}
 */
public class TomitaParser {

    /**
     * Кодировка текстовых буферов, используемых для обмена с нативной стороной
     */
    private static final Charset CHARSET = StandardCharsets.UTF_8;

    /**
     * "Макс." размер символа в байтах в UTF-8 (бывает больше, но редко, в данном случае не критично)
     */
    private static final int CHAR_MAX_EXPECTED_BYTES_UTF = 2;

    private long address;
    private File confDir;
    private String[] argv;

    /**
     * Создает новый парсер.
     *
     * @param confDir путь к директории с конфигурационными файлами парсера
     * @param argv параметры конфигурации (аналогичные тем, которые передаются при запуске утилиты
     */
    public TomitaParser(File confDir, String[] argv) {
        TomitaParserInternal.ensureNativeLibLoaded();

        Objects.requireNonNull(confDir, "Please specify configuration directory");
        Objects.requireNonNull(argv, "Please specify configuration parameters");

        this.confDir = confDir;
        this.argv = argv;

        address = TomitaParserInternal.create(argv, confDir.getAbsolutePath() + File.separator);
    }

    public String[] getArgv() {
        return argv;
    }

    public File getConfDir() {
        return confDir;
    }

    /**
     * Парсинг. Принимает на вход документы для парсинга, путь
     * к директории с конфигами и параметры конфигурации, идентичные тем, которые принимает утилита
     * командной строки ./tomita_parser.
     *
     * @param input входные документы, которые будут парсится
     * @return xml с результатами работы парсера
     * @throws IllegalStateException проблемы вызова парсера (версии so и jar не соответствуют друг другу или
     * по какой-то не было причине не удалось получить результаты парсинга)
     * @throws RuntimeException ошибка на нативной стороне
     */
    public String parse(Collection<String> input) {
        checkAlive();

        // Считаем capacity, чтобы при формировании буфера не было лишних переаллокаций.
        int maxBufferSize = CHAR_MAX_EXPECTED_BYTES_UTF * input.stream().mapToInt(String::length).sum();
        ByteArrayOutputStream buffer = new ByteArrayOutputStream(maxBufferSize);
        int[] inputLengths = new int[input.size()];

        int i = 0;
        for (String inputStr : input) {
            byte[] inputStrBytes = inputStr.getBytes(CHARSET);
            writeBytes(buffer, inputStrBytes);
            writeBytes(buffer, new byte[] {0}); // 0-terminator для сишных функций
            inputLengths[i++] = inputStrBytes.length + 1;
        }

        byte[] result = TomitaParserInternal.parse(this.address, buffer.toByteArray(), inputLengths);

        if (result == null || result.length == 0) {
            throw new IllegalStateException("Missing or empty output from native call");
        }

        return new String(result, CHARSET);
    }

    public void dispose() {
        checkAlive();

        TomitaParserInternal.dispose(this.address);

        this.address = 0L;
    }

    private void writeBytes(ByteArrayOutputStream buffer, byte[] inputStrBytes) {
        try {
            buffer.write(inputStrBytes);
        } catch (IOException e) {
            // Это исключение никогда не произойдет
            throw new RuntimeException("Failed to write to memory buffer", e);
        }
    }

    private void checkAlive() {
        if (this.address == 0) {
            throw new IllegalStateException("The current TomitaParser instance is disposed already");
        }
    }
}
