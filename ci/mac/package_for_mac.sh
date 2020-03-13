# 获取绝对路径，保证其他目录执行此脚本依然正确
{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output
# 设置当前目录，cd的目录影响接下来执行程序的工作目录
old_cd=$(pwd)
cd $(dirname "$0")

echo
echo
echo ---------------------------------------------------------------
echo pip install requirements
echo ---------------------------------------------------------------

pip install -r $script_path/package/requirements.txt
if [ $? -ne 0 ] ;then
    echo "pip install requirements failed"
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo create package
echo ---------------------------------------------------------------

python $script_path/package/package.py
if [ $? -ne 0 ] ;then
    echo "create package failed"
    exit 1
fi

# 恢复当前目录
cd $old_cd
exit 0
