# HTTP BAIDU ACCESSS TOKEN

获取百度API服务中的access token

参考Python代码示例
```python
import requests
import json


def main():
        
    url = "https://aip.baidubce.com/oauth/2.0/token?client_id=MIm8tZ****FlIBkA&client_secret=SLS0Pr****9kBOI0&grant_type=client_credentials"
    
    payload = json.dumps("")
    headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
    }
    
    response = requests.request("POST", url, headers=headers, data=payload)
    
    print(response.text)
    

if __name__ == '__main__':
    main()
```