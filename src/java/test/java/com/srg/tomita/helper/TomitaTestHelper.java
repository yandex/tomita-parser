package com.srg.tomita.helper;

import com.opencsv.CSVReader;
import com.srg.tomita.TomitaParser;
import com.srg.tomita.TomitaPooledParser;
import org.apache.commons.io.FileUtils;
import org.junit.Assert;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class TomitaTestHelper {
    private String resourceName;
    private Collection<String> documents = new ArrayList<>();
    private int documentsSize;

    private long testStartMs;
    private long totalProcessedBytes;
    private File confDir;
    private File inputDir;

    private List<TomitaParser> parsers = new ArrayList<>();
    private List<TomitaPooledParser> pooledParsers = new ArrayList<>();
    private DocumentBuilder dBuilder;

    public TomitaTestHelper(String resourceName, String... filesToExclude) {
        try {
            setupForResource(resourceName, filesToExclude);
        } catch (Exception e) {
            throw new RuntimeException("Failed to setup resources for test", e);
        }
    }

    public String getResourceName() {
        return resourceName;
    }

    public Collection<String> getDocuments() {
        return documents;
    }

    public int getDocumentsSize() {
        return documentsSize;
    }

    private void setupForResource(String resourceName, String... filesToExclude) throws Exception {
        this.documents.clear();
        totalProcessedBytes = 0L;

        this.resourceName = resourceName;

        confDir = new File(new File("src/java/test/resources"), resourceName);
        if (!confDir.exists()) {
            throw new IllegalStateException("The test configuration path is not valid: " + confDir.getAbsolutePath());
        }
        inputDir = new File(confDir, "input");
        if (!inputDir.exists()) {
            throw new IllegalStateException("The test input data path is not valid: " + inputDir.getAbsolutePath());
        }

        Set<String> excludeSet = new HashSet<>(Arrays.asList(filesToExclude));
        File[] files = inputDir.listFiles();
        if (files == null) {
            throw new RuntimeException("Missing input");
        }
        for (File file : files) {
            if (!excludeSet.contains(file.getName())) {
                if (file.getName().endsWith(".csv")) {
                    documents.addAll(parseCsv(file));
                } else {
                    documents.add(FileUtils.readFileToString(file));
                }
            }
        }
        documentsSize = documents.stream().mapToInt(doc -> doc.getBytes().length).sum();

        testStartMs = System.currentTimeMillis();

        DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
        dBuilder = dbFactory.newDocumentBuilder();
    }

    public void dispose() {
        this.parsers.forEach(TomitaParser::dispose);
    }

    public TomitaParser createParser() {
        TomitaParser parser = new TomitaParser(confDir, new String[]{"config.proto"});
        this.parsers.add(parser);
        return parser;
    }

    public TomitaPooledParser createPooledParser(int poolSize) {
        TomitaPooledParser parser = new TomitaPooledParser(poolSize, confDir, new String[] {"config.proto"});
        this.pooledParsers.add(parser);
        return parser;
    }

    public void assertResponse(String response) {
        Assert.assertTrue(response.startsWith("<?xml version='1.0' encoding='utf-8'?>"));

        Document doc;
        try {
            Reader reader = new StringReader(response);
            InputSource is = new InputSource(reader);
            is.setEncoding(StandardCharsets.UTF_8.name());

            doc = dBuilder.parse(is);
        } catch (SAXException e) {
            throw new RuntimeException("Xml parse exception", e);
        } catch (IOException e) {
            throw new RuntimeException("String read exception (impossible)", e);
        }
        Element rootNode = doc.getDocumentElement();
        rootNode.normalize();
        Assert.assertEquals("fdo_objects", rootNode.getNodeName());

        int docCount = 0;
        for (int i = 0; i < rootNode.getChildNodes().getLength(); i++) {
            Node docNode = rootNode.getChildNodes().item(i);
            if ("document".equals(docNode.getNodeName())) {
                Attr urlAttr = (Attr) docNode.getAttributes().getNamedItem("url");
                String message = String.format("Check document index " + docCount);
                Assert.assertEquals(message, String.valueOf(docCount++), urlAttr.getValue());
            }
        }

        Assert.assertEquals(documents.size(), docCount);
    }

    public void logSpeed() {
        double processedMb = this.totalProcessedBytes / Math.pow(1024, 2);
        long durationMs = System.currentTimeMillis() - testStartMs;
        double speedMbPerS = 1000* processedMb / durationMs;

        System.out.format("Total Processed: %.1f Mb%n", processedMb);
        System.out.format("Speed: %.2f Mb/s%n", speedMbPerS);
    }

    public void setTotalProcessedBytes(long totalProcessedBytes) {
        this.totalProcessedBytes += totalProcessedBytes;
    }

    private List<String> parseCsv(File file) throws IOException {
        List<String> documents = new ArrayList<>();
        try (CSVReader csvReader = new CSVReader(new FileReader(file))) {
            String[] nextLine;
            while ((nextLine = csvReader.readNext()) != null) {
                if (!"id".equals(nextLine[0])) {
                    documents.add((nextLine.length < 2 ? "" : nextLine[1]) + " documentid " + nextLine[0]);
                }
            }
        }
        return documents;
    }
}
