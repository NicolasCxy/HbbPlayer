package com.hbb.player;

import org.junit.Test;

import java.util.ArrayList;
import java.util.HashMap;

import static org.junit.Assert.*;

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class ExampleUnitTest {

    private ArrayList<User> testList = new ArrayList<>();
    private HashMap<String, User> testMap = new HashMap<>();

    @Test
    public void addition_isCorrect() {
//        assertEquals(4, 2 + 2);

//        int a = 3601;
//        int b = a - 5;
//        System.out.println("-------------------------value:" + (3720 % 3600) + " ----------------------------------------");

        String test1 = "2001837598";
        String test2 = "2001837598_2";

        System.out.println("-------------------------value11:" + test2.contains(test1)+ " ----------------------------------------");
//        User user = new User();
//        testList.add(user);
//        testMap.put("1", user);
//        System.out.println("listStr: " + testList.toString());
//        System.out.println("MapStr: " + testMap.toString());
//        System.out.println("-----------------------------------------------------------------");
//
//        User user1 = testMap.get("1");
//        user1.setName("wwwwwwwwwwwwwww");
//        user1.setAge(999);
//
//        System.out.println("listStrAfter: " + testList.toString());
//        System.out.println("MapStrAfter: " + testMap.toString());

    }


    class User {
        private String name = "cc";
        private int age = 22;

        @Override
        public String toString() {
            return "User{" +
                    "name='" + name + '\'' +
                    ", age=" + age +
                    '}';
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public int getAge() {
            return age;
        }

        public void setAge(int age) {
            this.age = age;
        }
    }
}