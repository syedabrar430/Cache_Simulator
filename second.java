import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.*;

public class second {
    int cacheSize,associativity,blockSize,numSets,setIndexBits,offsetBits,totalMisses=0,totalHits=0;
    ArrayList<ArrayList<Integer>> cache;
    int[] setMisses,setHits;

    public second(int cacheSize, int associativity, int blockSize) {
        this.cacheSize = cacheSize;
        this.associativity = associativity;
        this.blockSize = blockSize;

        this.numSets = (cacheSize * 1024) / (associativity * blockSize);
        this.setIndexBits = (int) (Math.log(numSets) / Math.log(2));
        this.offsetBits = (int) (Math.log(blockSize) / Math.log(2));

        this.cache = new ArrayList<ArrayList<Integer>>(numSets);
        for (int i = 0; i < numSets; i++) {
            cache.add(new ArrayList<Integer>(associativity));
        }
        this.setMisses = new int[numSets];
        this.setHits = new int[numSets];
    }
    public void processTrace(List<String> trace) {
        for (String address : trace) {
            address=address.replace("0x","");
            int setIndex;
            int tag=-1;
            try {
                setIndex = getSetIndex(address);
                tag=(((int) Long.parseLong(address, 16))>>(setIndexBits+offsetBits))&((1<<(26-setIndexBits))-1);
            } catch (NumberFormatException e) {
                System.out.println("Invalid address format: ");
                continue;
            }
            ArrayList<Integer> set = cache.get(setIndex);
            if (set.contains(tag)) {
                totalHits++;
                setHits[setIndex]++;
                cache.get(setIndex).remove(set.indexOf(tag));
                cache.get(setIndex).add(tag);
            } else {
                totalMisses++;
                setMisses[setIndex]++;

                if (set.size() < associativity) {
                    cache.get(setIndex).add(tag);
                } else {
                    cache.get(setIndex).remove(0);
                    cache.get(setIndex).add(tag);
                }
            }
        }
    }
    private int getSetIndex(String address) {
        address=address.replace("0x","");
        int addressInt = (int) Long.parseLong(address, 16);
        int setIndexMask = (1 << setIndexBits) - 1;
        return (addressInt >> offsetBits) & setIndexMask;
    }
    public void printResults() {
        System.out.println("Accesses: " + (totalHits + totalMisses));
        System.out.println("Total Cache Hits: " + totalHits);
        System.out.println("Total Cache Misses: " + totalMisses);
        System.out.println("Set-Wise Hits & Misses");
        for (int i = 0; i < numSets; i++) {
            System.out.println("Set " + (i + 1) + ": Hits = " + setHits[i] + " Misses = " + setMisses[i]);
        }
    }
    public static void main(String[] args) {
        if (args.length < 4) {
            System.out.println("Insufficient arguments. Usage: java second <cacheSize> <associativity> <blockSize> <traceFile>");
            return;
        }
        int cacheSize = Integer.parseInt(args[0]);
        int associativity = Integer.parseInt(args[1]);
        int blockSize = Integer.parseInt(args[2]);
        String traceFilePath = args[3];


        List<String> trace = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader(traceFilePath))) {
            String line;
            while ((line = br.readLine()) != null) {
                String[] tokens = line.trim().split("\\s+");
        for (String token : tokens) {
            trace.add(token);
        }
            }
        } catch (IOException e) {
            System.out.println("Error reading trace file: " + e.getMessage());
            return;
        }

     second cacheSimulator = new second(cacheSize, associativity, blockSize);

        cacheSimulator.processTrace(trace);
        cacheSimulator.printResults();
    }
}