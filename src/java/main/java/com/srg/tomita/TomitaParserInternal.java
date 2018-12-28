package com.srg.tomita;

/**
 * Класс для непосредственного взаймодействия с динамической библиотекой.
 * <p>
 * Не предназначен для использования конечной стороной. Использовать {@link TomitaParser}.
 */
class TomitaParserInternal {

    /**
     * Название динамической библиотеки. Библиотека должна лежать в LD_LIBRARY_PATH, напр.
     * файл /usr/lib/libFactExtract-Parser-textminerlib_java.so
     */
    private static final String LIB_NAME = "FactExtract-Parser-textminerlib_java";

    private static String libInitLoadMessage;

    static {
        // Пытаемся загрузить динамическую библиотеку, и если не удается, то, пользуясь тем,
        // что static-блоки обеспечивают mutual exclusion, сохраняем сообщение об ошибке
        try {
            System.loadLibrary(LIB_NAME);
            libInitLoadMessage = "";
        } catch (UnsatisfiedLinkError e) { // мы не должны падать капитально, мешая класслоадерам делать свою работу,
            // в случае отсутствия динамических библиотек
            libInitLoadMessage = "Failed to load native library " + LIB_NAME + ". " + e.getMessage();
        }
    }

    static void ensureNativeLibLoaded() {
        String msg = libInitLoadMessage;
        if (!msg.isEmpty()) {
            throw new IllegalStateException(msg);
        }
    }

    /**
     * Создает новый парсер. На вход подаются параметры конфигурации парсера
     * и путь к директории с конфигами, который должен завершаться символом {@link java.io.File#separator}
     *
     * @param parameters параметры конфигурации
     * @param confDir путь к директории с конфигами, который должен завершаться символом {@link java.io.File#separator}
     */
    public static native long create(String[] parameters, String confDir);

    /**
     * Парсинг текста. На вход подается буфер с документами, записанными последовательно
     * друг за другом без разделителей, буфер с длинами документов,
     * который используется для определения границ документов в буфере.
     *
     * @param input буфер с документами, следующими друг за другом (без байтов-разделителей)
     * @param inputLengths длины документов
     * @return буфер с результатами парсинга в формате XML
     */
    public static native byte[] parse(long address, byte[] input, int[] inputLengths);

    public static native void dispose(long address);
}
