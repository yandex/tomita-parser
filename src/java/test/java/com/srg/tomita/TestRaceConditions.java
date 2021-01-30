package com.srg.tomita;

import com.srg.tomita.helper.TomitaTestHelper;
import org.junit.Test;

public class TestRaceConditions {
    @Test
    public void testOnSameConfig() {
        TomitaTestHelper helper = new TomitaTestHelper(TestPooledParser.class.getSimpleName());

        TomitaParser parser1 = helper.createParser();
        String response = parser1.parse(helper.getDocuments());
        helper.assertResponse(response);
        parser1.dispose();

        TomitaParser parser2 = helper.createParser();
        String response2 = parser2.parse(helper.getDocuments());
        helper.assertResponse(response2);
        parser2.dispose();

    }

    @Test
    public void testOnDifferentConfigs() {
        TomitaTestHelper helper = new TomitaTestHelper(TestPooledParser.class.getSimpleName());

        TomitaParser parser1 = helper.createParser();
        String response = parser1.parse(helper.getDocuments());
        helper.assertResponse(response);
        parser1.dispose();

        helper = new TomitaTestHelper(TestParser.class.getSimpleName());

        TomitaParser parser2 = helper.createParser();
        String response2 = parser2.parse(helper.getDocuments());
        helper.assertResponse(response2);
        parser2.dispose();
    }

    @Test
    public void testCase1() {
        TomitaTestHelper helper1 = new TomitaTestHelper(TestPooledParser.class.getSimpleName());
        TomitaTestHelper helper2 = new TomitaTestHelper(TestParser.class.getSimpleName());

        TomitaParser parser1 = helper1.createParser();
        TomitaParser parser2 = helper2.createParser();

        String response1 = parser1.parse(helper1.getDocuments());
        String response2 = parser2.parse(helper2.getDocuments());

        parser1.dispose();
        parser2.dispose();

        helper1.assertResponse(response1);
        helper2.assertResponse(response2);
    }

    @Test
    public void testCase2() {
        TomitaTestHelper helper1 = new TomitaTestHelper(TestPooledParser.class.getSimpleName());
        TomitaTestHelper helper2 = new TomitaTestHelper(TestParser.class.getSimpleName());

        TomitaParser parser1 = helper1.createParser();
        TomitaParser parser2 = helper2.createParser();

        String response1 = parser1.parse(helper1.getDocuments());
        parser1.dispose();

        String response2 = parser2.parse(helper2.getDocuments());
        parser2.dispose();

        helper1.assertResponse(response1);
        helper2.assertResponse(response2);
    }

    @Test
    public void testCase3() {
        TomitaTestHelper helper1 = new TomitaTestHelper(TestPooledParser.class.getSimpleName());
        TomitaTestHelper helper2 = new TomitaTestHelper(TestParser.class.getSimpleName());

        TomitaParser parser1 = helper1.createParser();

        String response1 = parser1.parse(helper1.getDocuments());


        TomitaParser parser2 = helper2.createParser();

        parser1.dispose();

        String response2 = parser2.parse(helper2.getDocuments());
        parser2.dispose();

        helper1.assertResponse(response1);
        helper2.assertResponse(response2);
    }
}
