import java.util.*;
class CopyCat
{
    public static void main (String []arg)
    {
        Scanner sc=new Scanner (System.in);
        int n=sc.nextInt();
        int count=0;
        while(n-- >0)
        {
            String s=sc.next();
            int freq[]=new int[26];
            for(char ch:s.toCharArray())
            {
                freq[ch-'a']++;
            }
            count=0;
            for(int f:freq)
            {
                count=Math.max(count,f);
            }
            int result=s.length()-count;
            System.out.println(result);
        }
    }
}